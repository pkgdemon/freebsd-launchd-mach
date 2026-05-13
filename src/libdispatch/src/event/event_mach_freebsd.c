/*
 * event_mach_freebsd.c — FreeBSD Mach IPC backend for libdispatch.
 *
 * Real DISPATCH_SOURCE_TYPE_MACH_RECV implementation on top of
 * libsystem_kernel's userland mach_msg syscall (which talks to mach.ko).
 * Doesn't enable HAVE_MACH: Apple's mach.c / event_kevent.c Mach blocks
 * use EVFILT_MACHPORG + Apple-internal Mach APIs (mach_port_construct,
 * vouchers, MIG, etc.) that aren't on FreeBSD. Instead we stay in the
 * non-Mach configuration and add this one source-type from outside the
 * gate. Apple-canonical contract: handler receives the message itself
 * via mach_msg(MACH_RCV_MSG, ...) and uses dispatch_source_get_handle
 * to retrieve the port name. We just tell the handler "wake up, there's
 * something to read."
 *
 * Approach: per-source detached pthread that calls
 *   mach_msg(MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT,
 *            send_size=0, rcv_size=0, port, 100ms, 0)
 * in a loop. rcv_size=0 with MACH_RCV_LARGE makes the kernel return
 * MACH_RCV_TOO_LARGE *without* consuming the message (ipc_mqueue.c's
 * "if MACH_RCV_LARGE option set, then don't take it off the queue").
 * On the no-msg→msg transition we call dispatch_source_merge_data so
 * libdispatch schedules the user's event handler on the target queue;
 * the handler then mach_msg's the message off itself. On TIMED_OUT
 * (queue drained — handler consumed it) we reset the transition flag.
 *
 * Lifecycle: the polling thread retains the dispatch_source_t once
 * source.c wires up du_owner_wref. That retain pins the unote (since
 * source dispose disposes the unote) so the thread can keep reading
 * du_ident / state safely. The thread exits when
 * dispatch_source_testcancel(ds) becomes true, or when mach_msg returns
 * something other than TOO_LARGE/TIMED_OUT (port destroyed, mach.ko
 * gone). On exit, dispatch_release breaks the retain and lets the
 * source / unote dealloc normally.
 *
 * SEND + the internal _dispatch_mach_type_{notification,send,recv,
 * reply} + _dispatch_xpc_type_sigterm stay as link-only stubs so libxpc
 * etc. still link; dispatch_source_create on a SEND type returns NULL.
 *
 * Gated on FreeBSD AND HAVE_MACH unset. On a future hypothetical
 * FreeBSD with HAVE_MACH (full XNU Mach userland) this file would be
 * excluded and Apple's mach.c / event_kevent.c definitions take over.
 */

#include "internal.h"

#if defined(__FreeBSD__) && !HAVE_MACH

#include <pthread.h>
#include <string.h>
#include <unistd.h>

/*
 * libdispatch's src/shims/mach.h provides "fake" mach typedefs
 * (mach_port_t, mach_msg_return_t, mach_msg_header_t = void *, ...) for
 * the non-HAVE_MACH build. Including libsystem_kernel's <mach/message.h>
 * here would collide on those typedefs (CI: "typedef redefinition with
 * different types"). So instead of pulling that header in, we declare
 * the minimum surface we need locally and rely on the runtime linker to
 * resolve mach_msg against libsystem_kernel.so — the symbol name and
 * ABI match regardless of how the function was prototyped in this TU.
 * The libdispatch shim already gives us mach_port_t, mach_msg_return_t,
 * and MACH_PORT_NULL.
 */
typedef unsigned int mach_msg_option_t;
typedef unsigned int mach_msg_size_t;
typedef unsigned int mach_msg_timeout_t;

#define MACH_RCV_MSG       0x00000002
#define MACH_RCV_LARGE     0x00000004
#define MACH_RCV_TIMEOUT   0x00000100
#define MACH_MSG_SUCCESS   0x00000000
#define MACH_RCV_TIMED_OUT 0x10004003
#define MACH_RCV_TOO_LARGE 0x10004004

extern mach_msg_return_t mach_msg(void *msg, mach_msg_option_t option,
    mach_msg_size_t send_size, mach_msg_size_t rcv_size,
    mach_port_t rcv_name, mach_msg_timeout_t timeout,
    mach_port_t notify);

#define DISPATCH_MACH_RECV_POLL_MS	100

#define DISPATCH_MACH_RECV_WREF_SPIN_LIMIT  5000  /* * 1ms = 5s */

/*
 * _dispatch_mach_recv_poll — per-source polling thread.
 *
 * arg is the dispatch_source_refs_s the dst_create allocated. Loops on
 * a 100ms peek; merges on no-msg→msg transition; exits on cancel or
 * unexpected mach_msg error. Detached, frees nothing — libdispatch's
 * _dispatch_unote_dispose frees the unote once our dispatch_release
 * drops the last ref on the source.
 */
static void *
_dispatch_mach_recv_poll(void *arg)
{
	dispatch_source_refs_t dr = arg;

	/*
	 * Capture port immediately. du_ident was set in dst_create before
	 * pthread_create — pthread_create's start barrier publishes it to
	 * us. After this read we don't touch dr until dispatch_retain pins
	 * the source, which closes the race window where a user could
	 * dispatch_release the source before dispatch_activate and trigger
	 * unote dispose before we've taken our reference.
	 */
	mach_port_t port = (mach_port_t)dr->du_ident;

	uintptr_t wref;
	int spins = 0;

	/*
	 * source.c sets du_owner_wref AFTER dst_create returns, so we may
	 * see 0 here on a tight race. Spin (1ms granularity, 5s cap) until
	 * the field is populated. __atomic_load_n forces a fresh read each
	 * iteration. If we time out, dispatch_source_create probably failed
	 * around us; just exit and let libdispatch free the unote.
	 */
	for (;;) {
		wref = __atomic_load_n(&dr->du_owner_wref, __ATOMIC_RELAXED);
		if (wref != 0) {
			break;
		}
		if (++spins > DISPATCH_MACH_RECV_WREF_SPIN_LIMIT) {
			return NULL;
		}
		usleep(1000);
	}

	dispatch_source_t ds = (dispatch_source_t)_dispatch_wref2ptr(wref);

	/*
	 * Pin the source. Until we _dispatch_release, ds and dr stay live —
	 * source dispose runs _dispatch_unote_dispose which frees dr. Our
	 * retain prevents that, so dr is safe to read for the duration of
	 * this function. (We don't actually touch dr again after this
	 * point, but the invariant matters if the loop is extended.)
	 */
	_dispatch_retain(ds);

	bool pending = false;

	while (!dispatch_source_testcancel(ds)) {
		/*
		 * Receive buffer: with rcv_size=0 the kernel returns
		 * MACH_RCV_TOO_LARGE without touching this buffer (just
		 * reports the message stayed on the queue). A small zeroed
		 * stack array is enough to give mach_msg a valid pointer.
		 */
		uint8_t hdrbuf[32];
		memset(hdrbuf, 0, sizeof(hdrbuf));

		mach_msg_return_t mr = mach_msg(hdrbuf,
		    MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT,
		    0,    /* send_size */
		    0,    /* rcv_size: forces TOO_LARGE without consume */
		    port,
		    DISPATCH_MACH_RECV_POLL_MS,
		    MACH_PORT_NULL);

		switch (mr) {
		case MACH_RCV_TOO_LARGE:
			/*
			 * A message is waiting. Fire merge_data only on the
			 * no-msg→msg edge: otherwise we'd re-fire every 100ms
			 * for the same message until the handler runs.
			 */
			if (!pending) {
				dispatch_source_merge_data(ds, 1);
				pending = true;
			}
			break;
		case MACH_RCV_TIMED_OUT:
			/*
			 * Queue is empty. If pending was set, the handler must
			 * have drained the message (otherwise TOO_LARGE would
			 * have repeated). Reset so the next message triggers a
			 * fresh edge.
			 */
			pending = false;
			break;
		default:
			/*
			 * Port destroyed, mach.ko unloaded, or other unexpected
			 * error. Give up. The user's handler will simply stop
			 * being woken; cancel/release tears the source down.
			 */
			goto out;
		}
	}

out:
	_dispatch_release(ds);
	return NULL;
}

/*
 * dst_create for DISPATCH_SOURCE_TYPE_MACH_RECV.
 *
 * handle is the mach port name (must be non-zero); mask is unused
 * (Apple's MACH_RECV mask bits like DISPATCH_MACH_SEND_DEAD don't apply
 * to plain RECV). Allocate a bare dispatch_source_refs_s (same shape
 * data_add uses for "userland-only" sources), spawn the polling thread
 * detached, return the unote.
 */
static dispatch_unote_t
_dispatch_source_type_mach_recv_create(dispatch_source_type_t dst,
		uintptr_t handle, uintptr_t mask)
{
	if (handle == 0 || mask != 0) {
		return DISPATCH_UNOTE_NULL;
	}

	dispatch_source_refs_t dr = _dispatch_calloc(1u, dst->dst_size);
	dr->du_type = dst;
	dr->du_filter = dst->dst_filter;
	dr->du_is_direct = true;
	dr->du_ident = (dispatch_unote_ident_t)handle;

	pthread_t th;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int err = pthread_create(&th, &attr, _dispatch_mach_recv_poll, dr);
	pthread_attr_destroy(&attr);
	if (err != 0) {
		free(dr);
		return DISPATCH_UNOTE_NULL;
	}

	return (dispatch_unote_t){ ._dr = dr };
}

/*
 * Real MACH_RECV source type. CUSTOM_ADD filter so register/unregister
 * stays in the userland-only path (event.c:144-150,185-189) — no
 * kqueue, no muxnote. SOURCE_ADD_DATA action so ds_pending_data
 * accumulates the per-message count we merge_data; the user's handler
 * sees dispatch_source_get_data() == number of messages woken since
 * last drain.
 */
const dispatch_source_type_s _dispatch_source_type_mach_recv = {
	.dst_kind       = "mach_recv (freebsd)",
	.dst_filter     = DISPATCH_EVFILT_CUSTOM_ADD,
	.dst_flags      = EV_UDATA_SPECIFIC|EV_CLEAR,
	.dst_action     = DISPATCH_UNOTE_ACTION_SOURCE_ADD_DATA,
	.dst_size       = sizeof(struct dispatch_source_refs_s),
	.dst_strict     = false,

	.dst_create     = _dispatch_source_type_mach_recv_create,
	.dst_merge_evt  = NULL,
};

/*
 * Link-only stubs for the remaining Mach source types. dispatch_source_
 * create on these returns NULL (caller gets DISPATCH_BAD_INPUT). They
 * exist purely so libxpc / configd / etc. that reference Apple's
 * full Mach source-type symbol set still link against
 * libsystem_dispatch.so on FreeBSD. Real implementations (send-dead
 * notifications, internal channel state, XPC sigterm hook) land if/when
 * we port the corresponding consumers.
 */
static dispatch_unote_t
_dispatch_source_type_mach_stub_create(
		dispatch_source_type_t dst DISPATCH_UNUSED,
		uintptr_t handle DISPATCH_UNUSED,
		uintptr_t mask DISPATCH_UNUSED)
{
	return DISPATCH_UNOTE_NULL;
}

static void
_dispatch_source_type_mach_stub_merge_evt(
		dispatch_unote_t du DISPATCH_UNUSED,
		uint32_t flags DISPATCH_UNUSED,
		uintptr_t data DISPATCH_UNUSED,
		pthread_priority_t pp DISPATCH_UNUSED)
{
}

#define _DISPATCH_MACH_FREEBSD_STUB_TYPE(name, kind) \
	const dispatch_source_type_s name = { \
		.dst_kind  = kind, \
		.dst_filter = 0, \
		.dst_flags  = 0, \
		.dst_action = DISPATCH_UNOTE_ACTION_PASS_DATA, \
		.dst_size   = sizeof(struct dispatch_source_refs_s), \
		.dst_create = _dispatch_source_type_mach_stub_create, \
		.dst_merge_evt = _dispatch_source_type_mach_stub_merge_evt, \
	}

/* Public: SEND notifications need MACH_NOTIFY_NO_SENDERS plumbing we
 * don't have yet. Stub. */
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_source_type_mach_send,
    "mach_send (freebsd-stub)");

/* Internal — referenced by Apple's mach.c / event_kevent.c. Stubs;
 * present so libxpc links. */
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_notification,
    "mach_notification (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_send,
    "mach_send_internal (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_recv,
    "mach_recv_internal (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_reply,
    "mach_reply (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_xpc_type_sigterm,
    "xpc_sigterm (freebsd-stub)");

#endif /* __FreeBSD__ && !HAVE_MACH */

/*
 * event_mach_freebsd.c — FreeBSD Mach IPC backend for libdispatch.
 *
 * STUB SPIKE (this commit). Provides definitions for the Mach source-type
 * symbols so consumers (libxpc, libnotify, configd, etc.) can LINK
 * against libsystem_dispatch.so without undefined-reference errors. The
 * dst_create callbacks return DISPATCH_UNOTE_NULL so dispatch_source_create
 * with one of these types returns NULL instead of crashing — the consumer
 * gets a clean failure it can detect. Real polling-thread receive (using
 * mach_msg_trap from libsystem_kernel) lands in a follow-up commit.
 *
 * Why a separate file: libdispatch's existing Mach machinery in
 * src/mach.c (~3,250 LOC) and src/event/event_kevent.c (Mach blocks,
 * lines 3233-3419) is gated on HAVE_MACH and uses Apple-specific Mach
 * APIs (mach_port_construct, vouchers, EVFILT_MACHPORT, etc.) that
 * aren't on FreeBSD. We don't enable HAVE_MACH; instead we provide the
 * minimum symbol surface here from outside the gate.
 *
 * Symbols provided (matching the names dispatch/source.h's public macros
 * reference + the internal names mach.c would use if HAVE_MACH were set):
 *   _dispatch_source_type_mach_send  ← public DISPATCH_SOURCE_TYPE_MACH_SEND
 *   _dispatch_source_type_mach_recv  ← public DISPATCH_SOURCE_TYPE_MACH_RECV
 *   _dispatch_mach_type_notification ← internal (used by mach.c)
 *   _dispatch_mach_type_send         ← internal
 *   _dispatch_mach_type_recv         ← internal
 *   _dispatch_mach_type_reply        ← internal
 *   _dispatch_xpc_type_sigterm       ← internal (XPC-only)
 *
 * Compiled on FreeBSD only (gated in src/CMakeLists.txt). On Apple this
 * file isn't compiled — Apple's mach.c + event_kevent.c definitions win.
 */

#include "internal.h"

#if defined(__FreeBSD__) && !HAVE_MACH

/*
 * Stub dst_create — returns DISPATCH_UNOTE_NULL. dispatch_source_create
 * with a MACH_* type will return NULL; consumers that check for NULL get
 * a clean failure path. Anything that uses the source unconditionally
 * will dereference NULL and crash — a clear signal that we haven't yet
 * implemented real Mach receive.
 */
static dispatch_unote_t
_dispatch_source_type_mach_freebsd_create(
		dispatch_source_type_t dst DISPATCH_UNUSED,
		uintptr_t handle DISPATCH_UNUSED,
		uintptr_t mask DISPATCH_UNUSED)
{
	return DISPATCH_UNOTE_NULL;
}

/*
 * Stub dst_merge_evt — never called because our dst_create returns NULL,
 * so no event delivery happens. Defined as a no-op to give the function
 * pointer somewhere valid to point.
 */
static void
_dispatch_source_type_mach_freebsd_merge_evt(
		dispatch_unote_t du DISPATCH_UNUSED,
		uint32_t flags DISPATCH_UNUSED,
		uintptr_t data DISPATCH_UNUSED,
		pthread_priority_t pp DISPATCH_UNUSED)
{
	/* no-op stub — see file-level comment */
}

#define _DISPATCH_MACH_FREEBSD_STUB_TYPE(name, kind) \
	const dispatch_source_type_s name = { \
		.dst_kind  = kind, \
		.dst_filter = 0, \
		.dst_flags  = 0, \
		.dst_action = DISPATCH_UNOTE_ACTION_PASS_DATA, \
		.dst_size   = sizeof(struct dispatch_source_refs_s), \
		.dst_create = _dispatch_source_type_mach_freebsd_create, \
		.dst_merge_evt = _dispatch_source_type_mach_freebsd_merge_evt, \
	}

/* Public symbols — referenced by dispatch/source.h's MACH_* macros. */
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_source_type_mach_send, "mach_send (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_source_type_mach_recv, "mach_recv (freebsd-stub)");

/* Internal symbols — referenced by Apple's mach.c / event_kevent.c
 * mach_recv_direct / xpc paths. We define them so future consumers
 * (libxpc) can link; until real implementation lands, they behave the
 * same as the public stubs. */
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_notification, "mach_notification (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_send,         "mach_send_internal (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_recv,         "mach_recv_internal (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_mach_type_reply,        "mach_reply (freebsd-stub)");
_DISPATCH_MACH_FREEBSD_STUB_TYPE(_dispatch_xpc_type_sigterm,       "xpc_sigterm (freebsd-stub)");

#endif // __FreeBSD__ && !HAVE_MACH

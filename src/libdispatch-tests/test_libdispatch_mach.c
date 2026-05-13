/*
 * test_libdispatch_mach — end-to-end DISPATCH_SOURCE_TYPE_MACH_RECV
 * round-trip on top of the real polling-thread backend in
 * src/libdispatch/src/event/event_mach_freebsd.c.
 *
 * Flow:
 *   1. mach_reply_port() → get a receive-right port via libsystem_kernel.
 *   2. dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, port, 0, q)
 *      → real backend allocates a polling pthread that peeks the port
 *        with mach_msg(MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TIMEOUT,
 *        rcv_size=0) every ~100ms. Must return non-NULL (stub used to
 *        return NULL — that path is the failure case now).
 *   3. dispatch_source_set_event_handler — handler calls
 *        mach_msg(MACH_RCV_MSG, ...) to consume the message and
 *        dispatch_semaphore_signal to release main.
 *   4. dispatch_activate(src) — arms the source.
 *   5. mach_msg(MACH_SEND_MSG) — send a self-message using
 *      MACH_MSG_TYPE_MAKE_SEND_ONCE on our own receive right.
 *   6. dispatch_semaphore_wait with 5s timeout — handler fires, signals.
 *   7. dispatch_source_cancel + dispatch_release.
 *
 * Exit codes:
 *   0 — handler fired, message consumed, clean teardown.
 *   1 — mach_reply_port returned NULL (mach.ko not loaded or syscall
 *       missing — earlier smoke marker would already have failed).
 *   2 — dispatch_source_create returned NULL (real backend allocation
 *       failed — pthread_create error, calloc, or invalid handle).
 *   3 — mach_msg send failed.
 *   4 — semaphore timed out — polling thread didn't notice the message,
 *       or merge_data didn't reach the handler. The interesting failure.
 *   5 — handler's receive returned an unexpected status.
 *
 * Marker contract with run.sh: prints "LIBDISPATCH-MACH-OK" on success
 * to match the CI expect grammar (tests/boot-test.sh:186). The stub
 * marker is being reused — the message just changes to reflect the
 * round-trip semantics.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <dispatch/dispatch.h>
#include <mach/mach_traps.h>
#include <mach/message.h>

#define TEST_TIMEOUT_NS  (5ull * 1000 * 1000 * 1000)  /* 5 seconds */
#define TEST_MSG_ID      0x4d414348              /* "MACH" */

/*
 * Handler-thread state: the port (handler reads it to mach_msg recv)
 * and a semaphore the main thread waits on. Captured by the handler
 * block via the global; libdispatch handler blocks here are function
 * pointers (set_event_handler_f), not Blocks, so closure capture would
 * need a context pointer plumb — globals are simpler for a smoke test.
 */
static mach_port_name_t      g_port;
static dispatch_semaphore_t  g_sem;
static volatile int          g_recv_status;   /* set by handler */
static volatile uint32_t     g_recv_id;       /* msgh_id from received msg */

static void
recv_handler(void *ctxt)
{
	(void)ctxt;

	/* Buffer big enough for header + minimal trailer space; the kernel
	 * also appends a mach_msg_audit_trailer_t-ish trailer on receive,
	 * which on ravynOS's port is ~76 bytes. Pad generously. */
	struct {
		mach_msg_header_t hdr;
		uint8_t trailer_pad[128];
	} msg;
	memset(&msg, 0, sizeof(msg));

	mach_msg_return_t mr = mach_msg(&msg.hdr,
	    MACH_RCV_MSG | MACH_RCV_TIMEOUT,
	    0,             /* send_size */
	    sizeof(msg),   /* rcv_size */
	    g_port,
	    0,             /* timeout: 0ms — message is already queued */
	    MACH_PORT_NULL);

	g_recv_status = (int)mr;
	g_recv_id     = (uint32_t)msg.hdr.msgh_id;
	dispatch_semaphore_signal(g_sem);
}

int
main(void)
{
	/* 1. Allocate a Mach port (receive right) via libsystem_kernel. */
	g_port = mach_reply_port();
	if (g_port == MACH_PORT_NULL) {
		printf("FAIL: mach_reply_port returned MACH_PORT_NULL\n");
		return 1;
	}
	printf("got port = 0x%x\n", g_port);

	/* 2. Create the dispatch source. Real backend (not stub) returns
	 *    non-NULL — that's the headline assertion this whole test
	 *    exists to verify. */
	dispatch_queue_t q = dispatch_get_global_queue(
	    DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_source_t src = dispatch_source_create(
	    DISPATCH_SOURCE_TYPE_MACH_RECV,
	    (uintptr_t)g_port, 0, q);
	if (src == NULL) {
		printf("FAIL: dispatch_source_create returned NULL\n");
		printf("(if real backend is active, this should never happen;\n");
		printf(" check pthread_create / calloc failure paths)\n");
		return 2;
	}
	printf("dispatch_source_create OK: src=%p\n", (void *)src);

	/* 3. Set handler + 4. activate. */
	g_sem = dispatch_semaphore_create(0);
	dispatch_source_set_event_handler_f(src, recv_handler);
	dispatch_activate(src);

	/* 5. Send a self-message via MAKE_SEND_ONCE on our receive right.
	 *    The kernel manufactures a one-shot send right from the receive
	 *    right we own, delivers the message to our own port. */
	mach_msg_header_t send_hdr;
	memset(&send_hdr, 0, sizeof(send_hdr));
	send_hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND_ONCE, 0);
	send_hdr.msgh_size = sizeof(send_hdr);
	send_hdr.msgh_remote_port = g_port;
	send_hdr.msgh_local_port  = MACH_PORT_NULL;
	send_hdr.msgh_id          = TEST_MSG_ID;

	mach_msg_return_t mr = mach_msg(&send_hdr,
	    MACH_SEND_MSG | MACH_SEND_TIMEOUT,
	    sizeof(send_hdr),  /* send_size */
	    0,                 /* rcv_size */
	    MACH_PORT_NULL,    /* rcv_name */
	    100,               /* timeout: 100 ms */
	    MACH_PORT_NULL);
	if (mr != MACH_MSG_SUCCESS) {
		printf("FAIL: mach_msg(SEND) returned 0x%x\n", (unsigned)mr);
		return 3;
	}
	printf("self-message sent\n");

	/* 6. Wait for the handler to wake us. Polling thread runs every
	 *    100ms — we should wake within a few cycles. 5s ceiling
	 *    forgives a slow CI VM but still flags a hung backend. */
	long timed_out = dispatch_semaphore_wait(g_sem,
	    dispatch_time(DISPATCH_TIME_NOW, TEST_TIMEOUT_NS));
	if (timed_out != 0) {
		printf("FAIL: semaphore timed out after 5s — handler never fired.\n");
		printf("(polling thread didn't see the message, or merge_data "
		    "didn't reach the handler)\n");
		return 4;
	}

	if (g_recv_status != MACH_MSG_SUCCESS) {
		printf("FAIL: handler mach_msg(RCV) returned 0x%x "
		    "(expected MACH_MSG_SUCCESS)\n", (unsigned)g_recv_status);
		return 5;
	}
	if (g_recv_id != TEST_MSG_ID) {
		printf("FAIL: msgh_id mismatch: got 0x%x, expected 0x%x\n",
		    g_recv_id, TEST_MSG_ID);
		return 5;
	}

	/* 7. Tear down. dispatch_source_cancel signals testcancel → polling
	 *    thread exits → dispatch_release releases the source. We don't
	 *    explicitly wait for the polling thread; libdispatch's source
	 *    dispose runs after the polling thread's _dispatch_release. */
	dispatch_source_cancel(src);
	dispatch_release(src);
	dispatch_release(g_sem);

	printf("LIBDISPATCH-MACH-OK: real Mach RECV backend round-trip "
	    "succeeded (msgh_id=0x%x)\n", g_recv_id);
	return 0;
}

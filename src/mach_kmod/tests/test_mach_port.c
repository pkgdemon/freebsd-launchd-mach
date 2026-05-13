/*
 * test_mach_port — exercise mach_port_allocate / _insert_right /
 * _deallocate against the new sysctls registered by mach.ko.
 *
 * Flow (mirrors the libxpc xpc_connection_create_mach_service path):
 *   1. mach_port_allocate(MACH_PORT_RIGHT_RECEIVE) → get a new port.
 *   2. mach_port_insert_right(name=port, poly=port, MAKE_SEND) →
 *      attach a send right onto the same port (this is the path
 *      libxpc uses for service endpoints).
 *   3. mach_msg(SEND, MAKE_SEND_ONCE, port) → verify the port is
 *      actually usable for IPC after the alloc/insert dance.
 *   4. mach_msg(RCV, port, timeout=0) → drain the message we just
 *      queued, confirms the receive right is live.
 *   5. mach_port_deallocate(port) → drop the send right we inserted.
 *
 * Exit codes:
 *   0 — all calls succeeded; round-trip via the freshly-allocated
 *       port worked.
 *   1 — mach_task_self() returned MACH_PORT_NULL (no Mach state).
 *   2 — mach_port_allocate failed.
 *   3 — mach_port_insert_right failed.
 *   4 — mach_msg(SEND) failed on the newly-minted port.
 *   5 — mach_msg(RCV) didn't drain the message we just sent.
 *   6 — mach_port_deallocate failed.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/message.h>

#define	TEST_MSG_ID	0x504f5254	/* "PORT" */

int
main(void)
{
	mach_port_name_t task = mach_task_self();
	if (task == MACH_PORT_NULL) {
		printf("FAIL: mach_task_self returned MACH_PORT_NULL\n");
		return (1);
	}
	printf("task = 0x%x\n", task);

	/* 1. allocate a fresh receive-right port. */
	mach_port_name_t port = MACH_PORT_NULL;
	kern_return_t kr = mach_port_allocate(task,
	    MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_allocate returned 0x%x\n", (unsigned)kr);
		return (2);
	}
	printf("mach_port_allocate OK: port = 0x%x\n", port);

	/* 2. insert a MAKE_SEND right onto the same name. */
	kr = mach_port_insert_right(task, port, port, MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_insert_right returned 0x%x\n", (unsigned)kr);
		return (3);
	}
	printf("mach_port_insert_right OK\n");

	/* 3. send a self-message via MAKE_SEND_ONCE on our receive right.
	 *    Mirrors test_libdispatch_mach's send path; this time the
	 *    port came from mach_port_allocate rather than mach_reply_port,
	 *    proving the new allocation is fully usable. */
	mach_msg_header_t send_hdr;
	memset(&send_hdr, 0, sizeof(send_hdr));
	send_hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND_ONCE, 0);
	send_hdr.msgh_size = sizeof(send_hdr);
	send_hdr.msgh_remote_port = port;
	send_hdr.msgh_local_port  = MACH_PORT_NULL;
	send_hdr.msgh_id          = TEST_MSG_ID;

	mach_msg_return_t mr = mach_msg(&send_hdr,
	    MACH_SEND_MSG | MACH_SEND_TIMEOUT,
	    sizeof(send_hdr), 0, MACH_PORT_NULL, 100, MACH_PORT_NULL);
	if (mr != MACH_MSG_SUCCESS) {
		printf("FAIL: mach_msg(SEND) returned 0x%x\n", (unsigned)mr);
		return (4);
	}
	printf("self-message sent\n");

	/* 4. drain the message. With timeout=0 the kernel polls — the msg
	 *    was just enqueued, so this should succeed immediately. */
	struct {
		mach_msg_header_t hdr;
		uint8_t trailer_pad[128];
	} rcv;
	memset(&rcv, 0, sizeof(rcv));
	mr = mach_msg(&rcv.hdr, MACH_RCV_MSG | MACH_RCV_TIMEOUT,
	    0, sizeof(rcv), port, 0, MACH_PORT_NULL);
	if (mr != MACH_MSG_SUCCESS) {
		printf("FAIL: mach_msg(RCV) returned 0x%x\n", (unsigned)mr);
		return (5);
	}
	if (rcv.hdr.msgh_id != TEST_MSG_ID) {
		printf("FAIL: msgh_id mismatch: got 0x%x, expected 0x%x\n",
		    (unsigned)rcv.hdr.msgh_id, TEST_MSG_ID);
		return (5);
	}
	printf("received msg, msgh_id = 0x%x\n", (unsigned)rcv.hdr.msgh_id);

	/* 5. drop the send right we inserted. The receive right stays
	 *    until process exit; that path is exercised implicitly by
	 *    the kernel's fdescfree cleanup. */
	kr = mach_port_deallocate(task, port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_deallocate returned 0x%x\n", (unsigned)kr);
		return (6);
	}
	printf("mach_port_deallocate OK\n");

	printf("MACH-PORT-OK: allocate/insert_right/deallocate round-trip "
	    "succeeded (msgh_id=0x%x)\n", (unsigned)rcv.hdr.msgh_id);
	return (0);
}

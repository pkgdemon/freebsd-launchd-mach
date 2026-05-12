/*
 * test_mach_msg — exercise mach_msg() from userland through libmach.
 *
 * The minimal-but-real test: allocate a port via mach_reply_port (gets
 * us a receive right in our IPC space), then call mach_msg with
 * MACH_RCV_MSG and timeout=0 on an empty queue. The kernel must
 * return MACH_RCV_TIMED_OUT (0x10004003). Anything else — success,
 * different error, panic — means the receive path is broken.
 *
 * This proves: the wired mach_msg_trap syscall is reachable; the
 * 7-arg sysent passes user→kernel correctly; mach_msg_overwrite_trap
 * → ipc_mqueue_receive is callable from userland context; the
 * receive logic correctly times out on an empty queue.
 *
 * Future C3 follow-up: a real send-to-self with a constructed kmsg,
 * verifying message contents survive the IPC round-trip.
 */
#include <stdio.h>
#include <string.h>

#include <mach/mach_traps.h>
#include <mach/message.h>

int
main(void)
{
	mach_port_name_t reply_port = mach_reply_port();
	if (reply_port == MACH_PORT_NULL) {
		printf("FAIL: mach_reply_port returned MACH_PORT_NULL\n");
		return (1);
	}
	printf("got reply_port = 0x%x\n", reply_port);

	mach_msg_header_t hdr;
	memset(&hdr, 0, sizeof(hdr));

	mach_msg_return_t r = mach_msg(&hdr,
	    MACH_RCV_MSG | MACH_RCV_TIMEOUT, /* receive, don't block */
	    0,            /* send_size: irrelevant for recv */
	    sizeof(hdr),  /* rcv_size */
	    reply_port,   /* rcv_name */
	    0,            /* timeout: 0 ms with TIMEOUT bit set = poll once */
	    MACH_PORT_NULL); /* notify */

	printf("mach_msg returned 0x%x\n", (unsigned)r);
	if (r == MACH_RCV_TIMED_OUT) {
		printf("PASS: empty queue timed out as expected\n");
		return (0);
	}
	printf("FAIL: expected MACH_RCV_TIMED_OUT (0x%x), got 0x%x\n",
	    MACH_RCV_TIMED_OUT, (unsigned)r);
	return (1);
}

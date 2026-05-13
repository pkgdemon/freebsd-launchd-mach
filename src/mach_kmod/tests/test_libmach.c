/*
 * test_libmach — exercise each libsystem_kernel entry point and assert
 * non-NULL ports.
 *
 * Usage: ./test_libmach
 *
 * Prints one line per call (for diagnostic visibility), then asserts
 * every port is non-NULL. Post-Phase-C2 lazy Mach init, every process
 * gets a valid task / thread / host / reply port on first request, so
 * MACH_PORT_NULL from any of these is a real failure.
 *
 * Exit codes:
 *   0 — all four traps returned valid ports
 *   1 — mach_task_self()    returned MACH_PORT_NULL
 *   2 — mach_thread_self()  returned MACH_PORT_NULL
 *   3 — mach_host_self()    returned MACH_PORT_NULL
 *   4 — mach_reply_port()   returned MACH_PORT_NULL
 */
#include <stdio.h>
#include <mach/mach_traps.h>

int
main(void)
{
	mach_port_name_t task   = mach_task_self();
	mach_port_name_t thread = mach_thread_self();
	mach_port_name_t host   = mach_host_self();
	mach_port_name_t reply  = mach_reply_port();

	printf("mach_task_self   = 0x%x\n", task);
	printf("mach_thread_self = 0x%x\n", thread);
	printf("mach_host_self   = 0x%x\n", host);
	printf("mach_reply_port  = 0x%x\n", reply);

	if (task   == MACH_PORT_NULL) return 1;
	if (thread == MACH_PORT_NULL) return 2;
	if (host   == MACH_PORT_NULL) return 3;
	if (reply  == MACH_PORT_NULL) return 4;
	return 0;
}

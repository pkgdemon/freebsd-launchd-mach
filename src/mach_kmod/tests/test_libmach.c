/*
 * test_libmach — exercise each libmach entry point and print results.
 *
 * Usage: ./test_libmach
 *
 * Prints one line per call:
 *   mach_reply_port  = 0x...
 *   mach_task_self   = 0x...
 *   mach_thread_self = 0x...
 *   mach_host_self   = 0x...
 *
 * Exit code 0 always — interpretation is up to the smoke harness.
 * A return of MACH_PORT_NULL (0) is valid for processes that pre-date
 * mach.ko's load (NULL-guard wrapper returns MACH_PORT_NULL by design;
 * lazy Mach init in Phase C2 will fix that).
 */
#include <stdio.h>
#include <mach/mach_traps.h>

int
main(void)
{
	printf("mach_reply_port  = 0x%x\n", mach_reply_port());
	printf("mach_task_self   = 0x%x\n", mach_task_self());
	printf("mach_thread_self = 0x%x\n", mach_thread_self());
	printf("mach_host_self   = 0x%x\n", mach_host_self());
	return (0);
}

/*
 * test_task_special_port — exercise task_get_special_port /
 * task_set_special_port against the new sysctls registered by mach.ko.
 *
 * Flow (the path the bootstrap server's port-discovery will follow):
 *   1. mach_port_allocate(RECEIVE) + insert_right(MAKE_SEND) — mint a
 *      port with both receive and send rights, like a service port.
 *   2. task_set_bootstrap_port(task, port) — store it in the task's
 *      TASK_BOOTSTRAP_PORT slot.
 *   3. task_get_bootstrap_port(task, &out) — read it back. The kernel
 *      ipc_port_copyout_send installs the send right into our space;
 *      the name we get back should reference the same underlying port
 *      as the one we set. (Apple's Mach guarantees the same kernel
 *      port → same name in a given space, so we just compare names.)
 *   4. mach_port_deallocate(port) on the original name.
 *
 * Exit codes:
 *   0 — alloc/set/get/dealloc all succeeded; get returned the same
 *       port name we set.
 *   1 — mach_port_allocate failed.
 *   2 — mach_port_insert_right failed.
 *   3 — task_set_bootstrap_port failed.
 *   4 — task_get_bootstrap_port failed.
 *   5 — get returned a different port name than set.
 *   6 — mach_port_deallocate failed.
 */
#include <stdio.h>
#include <stdint.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/task_special_ports.h>

int
main(void)
{
	mach_port_name_t task = mach_task_self();
	mach_port_name_t port = MACH_PORT_NULL;
	mach_port_name_t got  = MACH_PORT_NULL;
	kern_return_t kr;

	printf("task = 0x%x\n", task);

	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_allocate returned 0x%x\n", (unsigned)kr);
		return (1);
	}
	printf("allocated port = 0x%x\n", port);

	kr = mach_port_insert_right(task, port, port, MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_insert_right returned 0x%x\n", (unsigned)kr);
		return (2);
	}

	kr = task_set_bootstrap_port(task, port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: task_set_bootstrap_port returned 0x%x\n", (unsigned)kr);
		return (3);
	}
	printf("task_set_bootstrap_port OK\n");

	kr = task_get_bootstrap_port(task, &got);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: task_get_bootstrap_port returned 0x%x\n", (unsigned)kr);
		return (4);
	}
	printf("task_get_bootstrap_port returned 0x%x\n", got);

	if (got != port) {
		printf("FAIL: round-trip mismatch — set 0x%x, got 0x%x\n",
		    port, got);
		return (5);
	}

	kr = mach_port_deallocate(task, port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_deallocate returned 0x%x\n", (unsigned)kr);
		return (6);
	}

	printf("TASK-SPECIAL-PORT-OK: TASK_BOOTSTRAP_PORT set/get round-trip "
	    "succeeded (port=0x%x)\n", port);
	return (0);
}

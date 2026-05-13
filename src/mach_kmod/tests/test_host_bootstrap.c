/*
 * test_host_bootstrap — validates the Phase G2b fallback:
 * task_get_special_port(TASK_BOOTSTRAP_PORT) returns the
 * host-wide bootstrap port registered via host_set_special_port
 * when the calling task's per-task itk_bootstrap slot is null.
 *
 * Flow:
 *   1. Allocate a service-shaped port (RECEIVE + MAKE_SEND).
 *   2. Clear our task's itk_bootstrap slot via
 *      task_set_bootstrap_port(self, MACH_PORT_NULL) so we know
 *      the fallback path is what gets exercised.
 *   3. Confirm task_get_bootstrap_port returns MACH_PORT_NULL
 *      pre-registration (no host slot, no per-task slot).
 *   4. host_set_special_port(mach_host_self(), HOST_BOOTSTRAP_PORT,
 *      port) — register it host-wide.
 *   5. task_get_bootstrap_port(self, &got) — should now return a
 *      non-null name referring to the registered port (same task,
 *      so Mach gives us the existing name back).
 *   6. Assert got == port.
 *   7. Unset the host slot (host_set_special_port with
 *      MACH_PORT_NULL) so subsequent test programs in the same
 *      boot don't inherit our registration.
 *
 * Exit codes:
 *   0 — fallback round-trip succeeded.
 *   1 — mach_port_allocate / insert_right failed.
 *   2 — initial clear of per-task bootstrap slot failed.
 *   3 — pre-register task_get_bootstrap_port returned non-null
 *       (per-task slot wasn't cleared, or some other process
 *       already set the host slot).
 *   4 — host_set_special_port failed.
 *   5 — post-register task_get_bootstrap_port failed or returned
 *       the wrong port.
 *   6 — host_set_special_port(NULL) cleanup failed.
 */
#include <stdio.h>
#include <stdint.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/task_special_ports.h>
#include <mach/host_special_ports.h>

int
main(void)
{
	mach_port_name_t task = mach_task_self();
	mach_port_name_t host = mach_host_self();
	mach_port_name_t port = MACH_PORT_NULL;
	mach_port_name_t got  = MACH_PORT_NULL;
	kern_return_t kr;

	printf("task = 0x%x host = 0x%x\n", task, host);

	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_allocate: 0x%x\n", (unsigned)kr);
		return (1);
	}
	kr = mach_port_insert_right(task, port, port, MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_insert_right: 0x%x\n", (unsigned)kr);
		return (1);
	}
	printf("allocated host bootstrap port = 0x%x\n", port);

	/* 2. Clear per-task bootstrap slot so the fallback is the only
	 * thing under test. */
	kr = task_set_bootstrap_port(task, MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: task_set_bootstrap_port(NULL): 0x%x\n", (unsigned)kr);
		return (2);
	}

	/* 3. Should be null both per-task AND host-wide at this point. */
	kr = task_get_bootstrap_port(task, &got);
	if (kr != KERN_SUCCESS || got != MACH_PORT_NULL) {
		printf("FAIL: pre-register task_get_bootstrap_port: kr=0x%x got=0x%x\n",
		    (unsigned)kr, got);
		return (3);
	}
	printf("pre-register task_get_bootstrap_port = 0x0 (good)\n");

	/* 4. Publish host-wide. */
	kr = host_set_special_port(host, HOST_BOOTSTRAP_PORT, port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: host_set_special_port: 0x%x\n", (unsigned)kr);
		return (4);
	}
	printf("host_set_special_port OK\n");

	/* 5. The fallback should kick in. */
	kr = task_get_bootstrap_port(task, &got);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: post-register task_get_bootstrap_port: 0x%x\n",
		    (unsigned)kr);
		return (5);
	}
	printf("post-register task_get_bootstrap_port = 0x%x\n", got);
	if (got != port) {
		printf("FAIL: fallback returned 0x%x, expected 0x%x\n",
		    got, port);
		return (5);
	}

	/* 7. Unset the host slot so we don't bleed into other tests. */
	kr = host_set_special_port(host, HOST_BOOTSTRAP_PORT, MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: host_set_special_port(NULL): 0x%x\n", (unsigned)kr);
		return (6);
	}

	printf("HOST-BOOTSTRAP-OK: host-bootstrap fallback works "
	    "(per-task null → host port 0x%x)\n", port);
	return (0);
}

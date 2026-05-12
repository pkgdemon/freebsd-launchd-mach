/*
 * mach_test.c — kernel-internal Mach IPC sanity tests via sysctl.
 *
 * Exposes:
 *
 *   mach.test_port_lifecycle  (RW)
 *     Allocates N ports via ipc_port_alloc_special, deallocates them,
 *     returns the count successfully cycled. Stresses ipc_port_alloc /
 *     port refcount / ipc_object_zone allocation. N defaults to 100;
 *     override by writing it (clamped at 100000).
 *
 * A "full Mach IPC round-trip" test (allocate port, send message,
 * receive message, verify contents) was attempted but found to need a
 * Mach-aware thread context: ipc_mqueue_send routes special-port
 * messages through ipc_kobject_server which expects specific message
 * formats (the simplest test panics with "strange destination rights"
 * because a raw msgh_bits=MAKE_SEND_ONCE doesn't satisfy the
 * kobject-server contract), and ipc_mqueue_receive requires
 * thread->ith_object to be pre-set. Building that context from a
 * sysctl handler is bigger than the test is worth.
 *
 * The port_lifecycle test still validates the most-likely-to-break
 * primitives: zone allocation under the IPC subsystem (uma_zalloc,
 * ipc_object_zone), port refcount lifecycle, and ipc_space_kernel
 * being usable as a context. A full IPC round-trip becomes a Phase C
 * concern, exercised properly via wired userland syscalls.
 *
 * Out-of-tree-only: this file is not part of the ravynOS source tree;
 * it's an additional test surface we ship in our extraction.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>

#include <sys/mach/mach_types.h>
#include <sys/mach/ipc/ipc_port.h>
#include <sys/mach/ipc/ipc_space.h>

SYSCTL_DECL(_mach);

#define	TEST_PORTS_DEFAULT	100
#define	TEST_PORTS_MAX		100000

static int test_ports_n = TEST_PORTS_DEFAULT;

static int
mach_test_port_lifecycle(SYSCTL_HANDLER_ARGS)
{
	int n, ok, i;
	ipc_port_t *ports;

	if (req->newptr != NULL) {
		int v;
		int error = SYSCTL_IN(req, &v, sizeof(v));
		if (error)
			return (error);
		if (v < 1 || v > TEST_PORTS_MAX)
			return (EINVAL);
		test_ports_n = v;
		return (0);
	}

	n = test_ports_n;
	ports = malloc(sizeof(ipc_port_t) * n, M_TEMP, M_WAITOK | M_ZERO);

	ok = 0;
	for (i = 0; i < n; i++) {
		ports[i] = ipc_port_alloc_special(ipc_space_kernel);
		if (ports[i] != IP_NULL)
			ok++;
	}

	for (i = 0; i < n; i++) {
		if (ports[i] != IP_NULL)
			ipc_port_dealloc_special(ports[i], ipc_space_kernel);
	}

	free(ports, M_TEMP);

	printf("mach_test: port_lifecycle alloc/dealloc %d of %d\n", ok, n);
	return (SYSCTL_OUT(req, &ok, sizeof(ok)));
}

SYSCTL_PROC(_mach, OID_AUTO, test_port_lifecycle,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MPSAFE,
    NULL, 0, mach_test_port_lifecycle, "I",
    "Alloc/dealloc N kernel Mach ports; returns successful count");

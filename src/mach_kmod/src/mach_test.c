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
#include <sys/mach/ipc/ipc_kmsg.h>
#include <sys/mach/ipc/ipc_mqueue.h>

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

/*
 * mach.test_in_kernel_mqueue
 *
 * Lower-level Mach IPC primitive test: exercises the kmsg/mqueue
 * plumbing without going through ipc_mqueue_send or ipc_mqueue_receive.
 * Those high-level entry points need a Mach-aware thread context
 * (ith_object, ith_msg, etc.) that's hard to fabricate from a sysctl
 * handler — see the file header comment.
 *
 * What this DOES test:
 *   - ipc_kmsg_alloc returns a valid buffer from ipc_kmsg_zone
 *   - ipc_kmsg_enqueue links a kmsg into a port's ip_messages queue
 *   - ipc_kmsg_queue_first/empty correctly reflect queue state
 *   - ipc_kmsg_dequeue unlinks the kmsg
 *   - ipc_kmsg_free returns the buffer to the zone
 *   - Port + queue tear down cleanly via ipc_port_dealloc_special
 *
 * What this DOES NOT test:
 *   - msgh_bits semantics / kobject routing (skipped by bypassing send)
 *   - receive-side blocking / waiter wakeup (skipped by bypassing receive)
 *
 * Returns the number of (alloc-enqueue-verify-dequeue-free) cycles
 * that completed without an assertion or NULL pointer along the way.
 */
#define	TEST_MQUEUE_DEFAULT	100
#define	TEST_MQUEUE_MAX		10000

static int test_mqueue_n = TEST_MQUEUE_DEFAULT;

static int
mach_test_in_kernel_mqueue(SYSCTL_HANDLER_ARGS)
{
	int n, ok, i;

	if (req->newptr != NULL) {
		int v;
		int error = SYSCTL_IN(req, &v, sizeof(v));
		if (error)
			return (error);
		if (v < 1 || v > TEST_MQUEUE_MAX)
			return (EINVAL);
		test_mqueue_n = v;
		return (0);
	}

	n = test_mqueue_n;
	ok = 0;
	for (i = 0; i < n; i++) {
		ipc_port_t port;
		ipc_kmsg_t kmsg;
		ipc_kmsg_queue_t q;

		port = ipc_port_alloc_special(ipc_space_kernel);
		if (port == IP_NULL)
			continue;

		kmsg = ipc_kmsg_alloc(IKM_SAVED_MSG_SIZE);
		if (kmsg == IKM_NULL) {
			ipc_port_dealloc_special(port, ipc_space_kernel);
			continue;
		}
		ikm_init(kmsg, IKM_SAVED_MSG_SIZE);
		ikm_set_header(kmsg, sizeof(mach_msg_header_t));

		q = &port->ip_messages.imq_messages;
		if (!ipc_kmsg_queue_empty(q))
			goto cleanup;
		ipc_kmsg_enqueue(q, kmsg);
		if (ipc_kmsg_queue_empty(q))
			goto cleanup;
		if (ipc_kmsg_queue_first(q) != kmsg)
			goto cleanup;
		if (ipc_kmsg_dequeue(q) != kmsg)
			goto cleanup;
		if (!ipc_kmsg_queue_empty(q))
			goto cleanup;

		ok++;
cleanup:
		ipc_kmsg_free(kmsg);
		ipc_port_dealloc_special(port, ipc_space_kernel);
	}

	printf("mach_test: in_kernel_mqueue %d of %d cycles OK\n", ok, n);
	return (SYSCTL_OUT(req, &ok, sizeof(ok)));
}

SYSCTL_PROC(_mach, OID_AUTO, test_in_kernel_mqueue,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MPSAFE,
    NULL, 0, mach_test_in_kernel_mqueue, "I",
    "Enqueue/dequeue N kmsgs through a port's mqueue (no send/recv); "
    "returns successful cycle count");

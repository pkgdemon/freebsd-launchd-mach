/*
 * mach_stats.c — read-only sysctl counters exposing live Mach state.
 *
 * Exposes:
 *
 *   mach.stats.ports_in_use   (RD)
 *     Current count of allocated kernel Mach ports. Sourced from
 *     uma_zone_get_cur(ipc_object_zones[IOT_PORT]). The legacy
 *     `port_count` global in ipc_port.c is only compiled under
 *     MACH_ASSERT, which we don't define — the UMA zone gives us
 *     the same answer without the debug overhead.
 *
 *   mach.stats.kmsgs_in_use   (RD)
 *     Current count of allocated kmsg zone entries — i.e. in-flight
 *     Mach messages. Read from uma_zone_get_cur(ipc_kmsg_zone).
 *
 * Out-of-tree-only: not part of the ravynOS source tree.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <vm/uma.h>

#include <sys/mach/ipc/ipc_object.h>	/* ipc_object_zones, IOT_PORT */

extern uma_zone_t ipc_kmsg_zone;	/* ipc_init.c */

SYSCTL_DECL(_mach);
static SYSCTL_NODE(_mach, OID_AUTO, stats, CTLFLAG_RD, 0,
    "Live Mach IPC counters");

static int
mach_stats_ports_in_use(SYSCTL_HANDLER_ARGS)
{
	int v = uma_zone_get_cur(ipc_object_zones[IOT_PORT]);

	return (SYSCTL_OUT(req, &v, sizeof(v)));
}

static int
mach_stats_kmsgs_in_use(SYSCTL_HANDLER_ARGS)
{
	int v = uma_zone_get_cur(ipc_kmsg_zone);

	return (SYSCTL_OUT(req, &v, sizeof(v)));
}

SYSCTL_PROC(_mach_stats, OID_AUTO, ports_in_use,
    CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_MPSAFE,
    NULL, 0, mach_stats_ports_in_use, "I",
    "Current count of allocated kernel Mach ports "
    "(uma_zone_get_cur on ipc_object_zones[IOT_PORT])");

SYSCTL_PROC(_mach_stats, OID_AUTO, kmsgs_in_use,
    CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_MPSAFE,
    NULL, 0, mach_stats_kmsgs_in_use, "I",
    "Current count of in-flight Mach messages (ipc_kmsg_zone allocations)");

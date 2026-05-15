/*
 * forward_stubs.c — stubs for the MIG-generated job_forward.defs
 * server handlers.
 *
 * launchd-842's job_forward.defs declares two `simpleroutine`
 * entries (look_up2_forward + parent_forward) with `serverprefix
 * job_mig_;`, so MIG generates job_forwardServer.c with calls to
 * job_mig_look_up2_forward() / job_mig_parent_forward(). Apple's
 * launchd implements them in the per-user/system-launchd cross-talk
 * path — when a per-user launchd's bootstrap lookup needs to reach
 * up to PID-1 launchd. Our port has no second launchd to forward to
 * yet, so the handlers reject with BOOTSTRAP_NOT_PRIVILEGED; the
 * MIG-generated server demux then sends that as the reply.
 *
 * Real implementations land when we wire up the per-user launchd
 * arrangement (post-PID-1).
 */

#include "core.h"
#include "launchd.h"
#include "bootstrap.h"
#include <mach/mach.h>
#include <uuid/uuid.h>

/*
 * MIG type aliases used in the generated signatures. name_t is a
 * fixed-size char buffer (matches `type name_t = c_string[128]` in
 * job_types.defs).
 */
typedef char name_t[128];

kern_return_t
job_mig_look_up2_forward(job_t j, mach_port_t rp, name_t servicename,
    pid_t targetpid, uuid_t instanceid, uint64_t flags)
{
	(void)j;
	(void)rp;
	(void)servicename;
	(void)targetpid;
	(void)instanceid;
	(void)flags;
	return BOOTSTRAP_NOT_PRIVILEGED;
}

kern_return_t
job_mig_parent_forward(job_t j, mach_port_t rp)
{
	(void)j;
	(void)rp;
	return BOOTSTRAP_NOT_PRIVILEGED;
}

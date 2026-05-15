/*
 * sys/resource.h — FreeBSD shim layered over the base <sys/resource.h>.
 *
 * macOS declares the per-thread / per-process I/O policy API
 * (setiopolicy_np + the IOPOL_* constants) in <sys/resource.h>.
 * launchd-842 uses it to throttle its background sync thread and to
 * tag jobs as DARWIN_BG. FreeBSD has no per-thread iopolicy knob, so
 * this shim pulls in the real <sys/resource.h> via #include_next and
 * then appends a no-op setiopolicy_np() plus the constant set —
 * launchd's calls become best-effort no-ops (they're already wrapped
 * in posix_assumes_zero(), i.e. non-fatal).
 */
#ifndef _FREEBSD_SHIM_SYS_RESOURCE_H_
#define _FREEBSD_SHIM_SYS_RESOURCE_H_

#include_next <sys/resource.h>

#ifndef IOPOL_TYPE_DISK
#define IOPOL_TYPE_DISK		0

#define IOPOL_SCOPE_THREAD	1
#define IOPOL_SCOPE_PROCESS	2
#define IOPOL_SCOPE_DARWIN_BG	3

#define IOPOL_DEFAULT		0
#define IOPOL_THROTTLE		1
#define IOPOL_PASSIVE		2
#define IOPOL_UTILITY		3
#define IOPOL_STANDARD		4

static __inline int
setiopolicy_np(int iotype, int scope, int policy)
{
	(void)iotype;
	(void)scope;
	(void)policy;
	return 0;
}
#endif /* IOPOL_TYPE_DISK */

#endif /* _FREEBSD_SHIM_SYS_RESOURCE_H_ */

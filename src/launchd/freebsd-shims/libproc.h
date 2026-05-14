/*
 * libproc.h — FreeBSD shim for Apple's <libproc.h>.
 *
 * Apple's libproc is the userland wrapper over the proc_info(2)
 * syscall family. launchd-842's libvproc.c reaches for the
 * dirty-process-tracking subset — proc_track_dirty() / proc_set_dirty()
 * / proc_get_dirty() — which is XNU's "this process has unsaved state"
 * mechanism used for jetsam / sudden-termination decisions.
 *
 * FreeBSD has no dirty-process-tracking equivalent. These are stubbed
 * to no-ops returning success: on FreeBSD, dirty-tracking simply
 * doesn't gate anything (no jetsam), so reporting "tracked / clean"
 * is the correct degraded behavior. A one-shot stderr note fires so
 * the gap is visible if a real consumer ever depends on it.
 */
#ifndef _LIBPROC_H_SHIM_
#define _LIBPROC_H_SHIM_

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* proc_track_dirty() flags — Apple's <sys/proc_info.h> values. */
#define PROC_DIRTY_TRACK		0x1
#define PROC_DIRTY_ALLOW_IDLE_EXIT	0x2
#define PROC_DIRTY_DEFER		0x4
#define PROC_DIRTY_LAUNCH_IN_PROGRESS	0x8

/* proc_get_dirty() result flags. */
#define PROC_DIRTY_TRACKED		0x1
#define PROC_DIRTY_ALLOWS_IDLE_EXIT	0x2
#define PROC_DIRTY_IS_DIRTY		0x4
#define PROC_DIRTY_LAUNCH_IS_IN_PROGRESS 0x8

#ifndef _LIBPROC_SHIM_WARN
#define _LIBPROC_SHIM_WARN(fn) do { \
	static int _w = 0; \
	if (!_w) { fprintf(stderr, \
	    "libproc: " fn ": stub (no dirty-tracking on FreeBSD)\n"); \
	    _w = 1; } \
} while (0)
#endif

static __inline int
proc_track_dirty(pid_t pid, uint32_t flags)
{
	(void)pid; (void)flags;
	_LIBPROC_SHIM_WARN("proc_track_dirty");
	return 0;
}

static __inline int
proc_set_dirty(pid_t pid, bool dirty)
{
	(void)pid; (void)dirty;
	_LIBPROC_SHIM_WARN("proc_set_dirty");
	return 0;
}

static __inline int
proc_get_dirty(pid_t pid, uint32_t *flags)
{
	(void)pid;
	if (flags != NULL)
		*flags = 0;
	_LIBPROC_SHIM_WARN("proc_get_dirty");
	return 0;
}

#endif /* !_LIBPROC_H_SHIM_ */

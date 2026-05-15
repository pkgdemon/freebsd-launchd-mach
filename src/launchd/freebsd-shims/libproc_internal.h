/*
 * libproc_internal.h — FreeBSD shim for Apple's <libproc_internal.h>,
 * the private companion to <libproc.h>.
 *
 * The proc_info(2) wrappers + struct vocabulary live in the
 * <libproc.h> shim and <sys/proc_info.h> shim; this header adds the
 * one private entry point launchd-842's core.c uses on top of them:
 * proc_setpcontrol().
 */
#ifndef _FREEBSD_SHIM_LIBPROC_INTERNAL_H_
#define _FREEBSD_SHIM_LIBPROC_INTERNAL_H_

#include <libproc.h>		/* proc_* wrappers + dirty-tracking flags */
#include <sys/proc_info.h>	/* PROC_SETPC_* controls */

/*
 * proc_setpcontrol() — sets how the kernel treats the calling
 * process under memory pressure (throttle / suspend / terminate).
 * FreeBSD has no per-process memory-pressure control knob, so this
 * is a no-op success: launchd's request is silently dropped.
 */
static __inline int
proc_setpcontrol(int control)
{
	(void)control;
	return 0;
}

#endif /* _FREEBSD_SHIM_LIBPROC_INTERNAL_H_ */

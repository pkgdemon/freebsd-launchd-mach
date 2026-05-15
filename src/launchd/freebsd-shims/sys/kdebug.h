/*
 * sys/kdebug.h — FreeBSD shim for Apple's <sys/kdebug.h>.
 *
 * macOS kdebug is the XNU trace facility (the kperf/ktrace backend).
 * launchd-842's runtime.c uses just two constants — DBG_FUNC_START /
 * DBG_FUNC_END — to OR into the trace-code argument of its
 * runtime_ktrace() wrapper, which routes elsewhere in this port.
 * FreeBSD has its own ktrace (different facility); supplying just
 * the two flag bits keeps the OR expression compiling. Values match
 * Apple's <sys/kdebug.h>.
 */
#ifndef _FREEBSD_SHIM_SYS_KDEBUG_H_
#define _FREEBSD_SHIM_SYS_KDEBUG_H_

#define DBG_FUNC_NONE	0
#define DBG_FUNC_START	1
#define DBG_FUNC_END	2

#endif /* _FREEBSD_SHIM_SYS_KDEBUG_H_ */

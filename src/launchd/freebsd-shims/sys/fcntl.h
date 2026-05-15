/*
 * sys/fcntl.h — FreeBSD shim layered over the base header.
 *
 * launchd-842's core.c opens watched paths with O_EVTONLY — a macOS
 * fcntl flag meaning "I want this fd only to receive vnode kqueue
 * notifications, not for read/write". FreeBSD has no equivalent; an
 * O_RDONLY open works for kqueue EVFILT_VNODE registration just fine.
 * Define O_EVTONLY to 0 so the bitwise-OR is a no-op.
 */
#ifndef _FREEBSD_SHIM_SYS_FCNTL_H_
#define _FREEBSD_SHIM_SYS_FCNTL_H_

#include_next <sys/fcntl.h>

#ifndef O_EVTONLY
#define O_EVTONLY	0
#endif

#endif /* _FREEBSD_SHIM_SYS_FCNTL_H_ */

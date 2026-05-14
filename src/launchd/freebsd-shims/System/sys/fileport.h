/*
 * System/sys/fileport.h — FreeBSD shim for Apple's fileport API.
 *
 * fileport_makeport(int fd, mach_port_t *port) wraps a file
 * descriptor as a Mach send-right; fileport_makefd(mach_port_t port)
 * is the inverse. Both are XNU-specific (xnu/bsd/kern/sys_filedesc.c)
 * with no FreeBSD equivalent — mach.ko has no fd<->Mach-port bridge.
 *
 * launchd-842's libvproc.c calls fileport_makefd() when unpacking
 * fds passed back from launchd. The stubs return -1/MACH_PORT_NULL
 * so the source parses and links; the gap only bites if a job
 * actually passes fds over Mach, which our Phase I1/I2 test surface
 * doesn't do. Same shim libxpc carries in src/libxpc/sys/fileport.h.
 */
#ifndef _SYSTEM_SYS_FILEPORT_H_SHIM_
#define _SYSTEM_SYS_FILEPORT_H_SHIM_

#include <mach/mach.h>		/* mach_port_t */
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

static __inline int
fileport_makeport(int fd, mach_port_t *port)
{
	(void)fd;
	if (port != NULL)
		*port = MACH_PORT_NULL;
	errno = ENOSYS;
	return -1;
}

static __inline int
fileport_makefd(mach_port_t port)
{
	(void)port;
	errno = ENOSYS;
	return -1;
}

#ifdef __cplusplus
}
#endif

#endif /* !_SYSTEM_SYS_FILEPORT_H_SHIM_ */

/*
 * sys/fileport.h — local shim for libxpc's `#include <sys/fileport.h>`.
 *
 * fileport_makeport(int fd, mach_port_t *port) wraps a file descriptor
 * as a Mach send-right; fileport_makefd(mach_port_t port) is the
 * inverse. Both are XNU-specific (xnu/bsd/kern/sys_filedesc.c) and
 * have no FreeBSD equivalent yet — they require kernel support to
 * convert between fd table entries and Mach IPC objects, which
 * mach.ko does not implement.
 *
 * libxpc's xpc_type.c references both in xpc_fd_create / xpc_fd_dup.
 * For now we just declare the prototypes so the header parses; the
 * resulting libxpc.so will have unresolved fileport_make* symbols
 * which only fire if a consumer actually calls xpc_fd_create — and
 * none of our current callers do. When fd-over-Mach is needed, we
 * implement fileport_makeport/makefd in libsystem_kernel.
 *
 * Reachable via the Makefile's -I${.CURDIR}, same pattern as
 * launch.h, Availability.h, uuid/uuid.h.
 */
#ifndef _LIBXPC_SYS_FILEPORT_H_SHIM_
#define _LIBXPC_SYS_FILEPORT_H_SHIM_

#include <mach/mach.h>		/* mach_port_t */

#ifdef __cplusplus
extern "C" {
#endif

int fileport_makeport(int fd, mach_port_t *port);
int fileport_makefd(mach_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* !_LIBXPC_SYS_FILEPORT_H_SHIM_ */

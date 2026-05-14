/*
 * stubs.c — local stubs for XNU-specific symbols libxpc references
 * but doesn't actually need on FreeBSD yet.
 *
 *   fileport_makeport(int fd, mach_port_t *port)
 *   fileport_makefd(mach_port_t port)
 *     XNU's filedesc → Mach send-right bridge. xpc_fd_create /
 *     xpc_fd_dup are the only callers; nothing in our current test
 *     surface actually invokes them, but the symbols need to resolve
 *     at link time so libxpc.so can be loaded.
 *
 *   vproc_transaction_begin(vproc_t)
 *   vproc_transaction_end(vproc_t, vproc_transaction_t)
 *     launchd vproc transaction API. xpc_transaction_begin / _end
 *     reach for these on Apple to coordinate launchd-managed exit;
 *     on FreeBSD without launchd-as-PID-1 there's nothing to
 *     coordinate, so they're no-ops.
 *
 * All stubs surface a runtime warning so anyone who actually depends
 * on the missing capability sees the gap early instead of debugging
 * silently-broken behavior. Promote a stub to a real implementation
 * (or a kernel feature) when a caller needs it.
 */

#include <errno.h>
#include <stdio.h>
#include <mach/mach.h>

#ifndef LIBXPC_STUB_WARN
#define LIBXPC_STUB_WARN(fn) \
	do { \
		static int warned = 0; \
		if (!warned) { \
			fprintf(stderr, \
			    "libxpc: " fn ": stub (not implemented on FreeBSD)\n"); \
			warned = 1; \
		} \
	} while (0)
#endif

int
fileport_makeport(int fd, mach_port_t *port)
{
	(void)fd;
	if (port != NULL)
		*port = MACH_PORT_NULL;
	LIBXPC_STUB_WARN("fileport_makeport");
	errno = ENOSYS;
	return -1;
}

int
fileport_makefd(mach_port_t port)
{
	(void)port;
	LIBXPC_STUB_WARN("fileport_makefd");
	errno = ENOSYS;
	return -1;
}

/* vproc opaque handles — declared opaque on purpose, matches Apple's
 * <vproc.h>. */
typedef void *vproc_t;
typedef void *vproc_transaction_t;

vproc_transaction_t
vproc_transaction_begin(vproc_t vp)
{
	(void)vp;
	return NULL;	/* opaque sentinel; vproc_transaction_end no-ops */
}

void
vproc_transaction_end(vproc_t vp, vproc_transaction_t txn)
{
	(void)vp;
	(void)txn;
}

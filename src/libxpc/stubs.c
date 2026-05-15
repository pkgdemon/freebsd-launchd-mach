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
#include <xpc/xpc.h>
#include <xpc/launchd.h>

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

/*
 * xpc_domain_server() — MIG-server demux generated from Apple's SDK
 * domain.defs. We don't ship domain.defs (it's in iOS SDK only); the
 * function is referenced only from launchd's runtime.c dispatch
 * fallback, where a "false" return tells the caller "not a domain
 * message". libxpc is the link unit that gathers the launchd-XPC
 * surface, so we plant the stub here. boolean_t (from mach/mach.h)
 * is unsigned int on this port.
 */
extern boolean_t xpc_domain_server(void *request, void *reply);

boolean_t
xpc_domain_server(void *request, void *reply)
{
	(void)request;
	(void)reply;
	LIBXPC_STUB_WARN("xpc_domain_server");
	return 0;	/* false — caller drops the message */
}

/*
 * xpc_copy_entitlements_for_pid() — declared in <xpc/launchd.h>;
 * Apple-backed by a syscall that asks csops(2) about a process's
 * code-signing entitlements. FreeBSD has no code-signing entitlement
 * surface, so we return NULL ("no entitlements").
 */
xpc_object_t
xpc_copy_entitlements_for_pid(pid_t pid)
{
	(void)pid;
	LIBXPC_STUB_WARN("xpc_copy_entitlements_for_pid");
	return NULL;
}

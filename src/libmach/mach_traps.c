/*
 * mach_traps.c — libmach implementation of the no-arg Mach trap family.
 *
 * Each entry point resolves its FreeBSD syscall number lazily from
 * `sysctl mach.syscall.<name>` on first call (cached in a static for
 * subsequent calls), then invokes the syscall. If the sysctl is
 * missing (mach.ko not loaded) or registration failed (negative number),
 * the function returns MACH_PORT_NULL — matching Apple's API contract
 * of MACH_PORT_NULL on no-resources.
 */

#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mach/mach_traps.h>

#define	NO_SYSCALL	(-1)

/*
 * resolve_syscall — read sysctl mach.syscall.<name>, return the
 * registered syscall number or NO_SYSCALL if unavailable.
 */
static int
resolve_syscall(const char *name)
{
	char oid[64];
	int num;
	size_t len = sizeof(num);

	if (snprintf(oid, sizeof(oid), "mach.syscall.%s", name) >=
	    (int)sizeof(oid))
		return (NO_SYSCALL);
	if (sysctlbyname(oid, &num, &len, NULL, 0) != 0)
		return (NO_SYSCALL);
	if (num < 0)
		return (NO_SYSCALL);
	return (num);
}

/*
 * Generate one trap entry point. Cache the resolved syscall number
 * in a static; first call does the sysctl lookup, subsequent calls
 * skip it. Thread-safety: the static is written at most once with a
 * value derived from a side-effect-free sysctl, so a racing reader
 * either sees NO_SYSCALL (and re-resolves) or sees the final value.
 */
#define	MACH_TRAP_NOARGS(name)						\
mach_port_name_t							\
name(void)								\
{									\
	static int num = NO_SYSCALL;					\
									\
	if (num == NO_SYSCALL) {					\
		num = resolve_syscall(#name);				\
		if (num == NO_SYSCALL)					\
			return (MACH_PORT_NULL);			\
	}								\
	return ((mach_port_name_t)syscall(num));			\
}

MACH_TRAP_NOARGS(mach_reply_port)

/*
 * Apple exposes the task-self trap as `task_self_trap` (the raw syscall
 * name) and `mach_task_self()` (a userland accessor often inlined to a
 * TLS-style global on Apple platforms). On FreeBSD, with no per-thread
 * Mach storage yet, the simplest mapping is: `mach_task_self()` IS the
 * syscall. Same shape for thread and host.
 */
mach_port_name_t
mach_task_self(void)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("task_self_trap");
		if (num == NO_SYSCALL)
			return (MACH_PORT_NULL);
	}
	return ((mach_port_name_t)syscall(num));
}

mach_port_name_t
mach_thread_self(void)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("thread_self_trap");
		if (num == NO_SYSCALL)
			return (MACH_PORT_NULL);
	}
	return ((mach_port_name_t)syscall(num));
}

mach_port_name_t
mach_host_self(void)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("host_self_trap");
		if (num == NO_SYSCALL)
			return (MACH_PORT_NULL);
	}
	return ((mach_port_name_t)syscall(num));
}

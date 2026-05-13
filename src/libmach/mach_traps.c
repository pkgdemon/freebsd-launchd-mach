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
#include <mach/mach_port.h>
#include <mach/message.h>
#include <mach/task_special_ports.h>
#include <mach/host_special_ports.h>

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

/*
 * mach_msg — send and/or receive a Mach message.
 *
 * Resolves mach.syscall.mach_msg_trap on first call. If the syscall
 * isn't registered, returns a non-zero indeterminate error code
 * (we don't have a defined "syscall unavailable" code in the public
 * mach_msg return-code space, so we return MACH_RCV_TIMED_OUT as a
 * placeholder error; callers should check that mach.ko is loaded
 * before relying on this).
 */
/*
 * mach_msg — 6-arg call into the wired mach_msg_trap.
 *
 * FreeBSD's libc syscall() only correctly shifts up to 6 args into
 * the kernel-syscall ABI registers (rdi, rsi, rdx, r10, r8, r9). The
 * Apple-shape mach_msg_trap has 7 args (msg, option, send_size,
 * rcv_size, rcv_name, timeout, notify); calling syscall(num, ...)
 * with the 7th arg silently garbles args 4-7 (kernel sees rcv_size=0,
 * rcv_name=0, timeout=passed-rcv_size).
 *
 * Workaround: pass only the first 6 args to the kernel. The kernel
 * wrapper sets notify=MACH_PORT_NULL. The `notify` arg here is
 * accepted for API compatibility but currently ignored — a future
 * `mach_msg2`-style wrapper or an inline-asm 7-arg syscall stub
 * can lift this restriction if real use needs notify.
 */
mach_msg_return_t
mach_msg(mach_msg_header_t *msg, mach_msg_option_t option,
    mach_msg_size_t send_size, mach_msg_size_t rcv_size,
    mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
    mach_port_name_t notify __attribute__((unused)))
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("mach_msg_trap");
		if (num == NO_SYSCALL)
			return (MACH_RCV_TIMED_OUT);
	}
	return ((mach_msg_return_t)syscall(num, msg, option,
	    send_size, rcv_size, rcv_name, timeout));
}

/*
 * Port-management traps. Same lazy-resolve pattern as the no-arg traps,
 * but each takes >0 args. All three return kern_return_t (0 = success).
 * On no mach.ko, return KERN_RESOURCE_SHORTAGE (6) — matches Apple's
 * "couldn't allocate" convention.
 *
 * `task` is always the caller's task port (use mach_task_self()).
 * The kernel-side wrappers ignore it and use current_task() — included
 * in the userland API for Apple-source-code compatibility.
 */
kern_return_t
mach_port_allocate(mach_port_name_t task, mach_port_right_t right,
    mach_port_name_t *name)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("mach_port_allocate");
		if (num == NO_SYSCALL)
			return (KERN_RESOURCE_SHORTAGE);
	}
	return ((kern_return_t)syscall(num, task, right, name));
}

kern_return_t
mach_port_deallocate(mach_port_name_t task, mach_port_name_t name)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("mach_port_deallocate");
		if (num == NO_SYSCALL)
			return (KERN_RESOURCE_SHORTAGE);
	}
	return ((kern_return_t)syscall(num, task, name));
}

kern_return_t
mach_port_insert_right(mach_port_name_t task, mach_port_name_t name,
    mach_port_t poly, mach_msg_type_name_t polyPoly)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("mach_port_insert_right");
		if (num == NO_SYSCALL)
			return (KERN_RESOURCE_SHORTAGE);
	}
	return ((kern_return_t)syscall(num, task, name, poly, polyPoly));
}

/*
 * Task special-port traps. `which` is TASK_BOOTSTRAP_PORT etc. from
 * <mach/task_special_ports.h>. task_get_bootstrap_port /
 * task_set_bootstrap_port are macros that pick TASK_BOOTSTRAP_PORT for
 * the `which` arg.
 */
kern_return_t
task_get_special_port(mach_port_name_t task, int which,
    mach_port_name_t *port)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("task_get_special_port");
		if (num == NO_SYSCALL)
			return (KERN_RESOURCE_SHORTAGE);
	}
	return ((kern_return_t)syscall(num, task, which, port));
}

kern_return_t
task_set_special_port(mach_port_name_t task, int which, mach_port_t port)
{
	static int num = NO_SYSCALL;

	if (num == NO_SYSCALL) {
		num = resolve_syscall("task_set_special_port");
		if (num == NO_SYSCALL)
			return (KERN_RESOURCE_SHORTAGE);
	}
	return ((kern_return_t)syscall(num, task, which, port));
}

/*
 * host_set_special_port — set a slot in the host's special-port
 * array. Routes through the task_set_special_port syscall: FreeBSD
 * reserves only 10 dynamic syscall slots (210-219) and we've used
 * them all, so the kernel-side task_set_special_port_trap handler
 * routes to realhost.special[] when `which == HOST_BOOTSTRAP_PORT`
 * (which doesn't collide with any TASK_* value). The bootstrap
 * server calls this once at startup to publish its receive port
 * host-wide; thereafter task_get_special_port(TASK_BOOTSTRAP_PORT)
 * falls back to it when the per-task itk_bootstrap slot is null.
 */
kern_return_t
host_set_special_port(mach_port_name_t host, int which, mach_port_t port)
{

	return (task_set_special_port(host, which, port));
}

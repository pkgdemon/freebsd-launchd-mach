/*
 * mach_syscall_wire.c — wire individual Mach syscalls to the FreeBSD
 * sysent table at module load.
 *
 * Why this exists. ravynOS's mach_module.c uses syscall_helper_register
 * on an array of SYSCALL_INIT_HELPER entries. Our compat shim sets
 * every SYS_<name> = NO_SYSCALL because we don't have Apple's
 * syscalls.master to assign real numbers. kern_syscall_helper_register
 * uses NO_SYSCALL as the end-of-array sentinel, so its loop exits at
 * the first entry without registering anything. Result: handlers are
 * loaded in the kmod but unreachable from userland.
 *
 * This file does it the right way for an out-of-tree module: call
 * syscall_register directly with offset = NO_SYSCALL to trigger
 * dynamic slot allocation. The kernel picks an unused syscall number;
 * we expose it via `sysctl mach.syscall.<name>` so userland test code
 * can find it.
 *
 * Phase B / Tier 1 B scope: wire the no-arg "trap" family
 * (mach_reply_port, task_self_trap, …). Each entry adds a per-syscall
 * sysent + offset + sysctl. Same NULL-guard wrapper pattern: if the
 * caller has no Mach state (p_machdata == NULL), return MACH_PORT_NULL
 * instead of NULL-faulting in current_task().
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/sysent.h>
#include <sys/proc.h>

#include <bsm/audit_kevents.h>		/* AUE_NULL */

/*
 * Forward-declare per-syscall arg structs. Full definitions come from
 * <sys/mach/_mach_sysproto.h> via mach_types.h's force-include in
 * compat_shim.h. We only need the prototype shape here because we
 * just take pointers to the sys_* handlers; the args structs aren't
 * dereferenced from this file.
 */
struct mach_reply_port_args;
struct task_self_trap_args;
struct thread_self_trap_args;
struct host_self_trap_args;

SYSCTL_DECL(_mach);
static SYSCTL_NODE(_mach, OID_AUTO, syscall, CTLFLAG_RW, 0,
    "Mach syscall numbers (dynamically allocated)");

/*
 * Handler defined in src/mach_traps.c. Signature:
 *   int sys_mach_reply_port(struct thread *, struct mach_reply_port_args *)
 *
 * The Mach handler reads td->td_retval[0] = mach_reply_port() and
 * returns 0. mach_reply_port() internally calls ipc_port_alloc against
 * current_task()->itk_space, which requires the calling process to
 * have a Mach task (p_machdata) set up via mach_task_init's
 * process_init eventhandler. Processes forked AFTER mach.ko loaded
 * satisfy this; processes that existed before will fault.
 */
int sys_mach_reply_port(struct thread *, struct mach_reply_port_args *);
int sys_task_self_trap(struct thread *, struct task_self_trap_args *);
int sys_thread_self_trap(struct thread *, struct thread_self_trap_args *);
int sys_host_self_trap(struct thread *, struct host_self_trap_args *);

/*
 * NULL-guard wrappers. The "no Mach state" path returns MACH_PORT_NULL
 * (0) — the same value Apple's API would return on no-resources —
 * instead of NULL-faulting in current_task() / itk_space. A future
 * Phase C lazy-init helper can promote the caller to a real
 * Mach-aware state.
 */
static int
sys_mach_reply_port_guarded(struct thread *td, struct mach_reply_port_args *uap)
{
	if (td->td_proc->p_machdata == NULL) {
		td->td_retval[0] = 0;	/* MACH_PORT_NULL */
		return (0);
	}
	return (sys_mach_reply_port(td, uap));
}

static int
sys_task_self_trap_guarded(struct thread *td, struct task_self_trap_args *uap)
{
	if (td->td_proc->p_machdata == NULL) {
		td->td_retval[0] = 0;	/* MACH_PORT_NULL */
		return (0);
	}
	return (sys_task_self_trap(td, uap));
}

static int
sys_thread_self_trap_guarded(struct thread *td, struct thread_self_trap_args *uap)
{
	if (td->td_proc->p_machdata == NULL) {
		td->td_retval[0] = 0;	/* MACH_PORT_NULL */
		return (0);
	}
	return (sys_thread_self_trap(td, uap));
}

static int
sys_host_self_trap_guarded(struct thread *td, struct host_self_trap_args *uap)
{
	if (td->td_proc->p_machdata == NULL) {
		td->td_retval[0] = 0;	/* MACH_PORT_NULL */
		return (0);
	}
	return (sys_host_self_trap(td, uap));
}

static struct sysent mach_reply_port_sysent = {
	.sy_narg	= 0,
	.sy_call	= (sy_call_t *)sys_mach_reply_port_guarded,
	.sy_auevent	= AUE_NULL,
	.sy_flags	= 0,
};

static struct sysent task_self_trap_sysent = {
	.sy_narg	= 0,
	.sy_call	= (sy_call_t *)sys_task_self_trap_guarded,
	.sy_auevent	= AUE_NULL,
	.sy_flags	= 0,
};

static struct sysent thread_self_trap_sysent = {
	.sy_narg	= 0,
	.sy_call	= (sy_call_t *)sys_thread_self_trap_guarded,
	.sy_auevent	= AUE_NULL,
	.sy_flags	= 0,
};

static struct sysent host_self_trap_sysent = {
	.sy_narg	= 0,
	.sy_call	= (sy_call_t *)sys_host_self_trap_guarded,
	.sy_auevent	= AUE_NULL,
	.sy_flags	= 0,
};

static int mach_reply_port_offset = NO_SYSCALL;
static struct sysent mach_reply_port_old_sysent;

static int task_self_trap_offset = NO_SYSCALL;
static struct sysent task_self_trap_old_sysent;

static int thread_self_trap_offset = NO_SYSCALL;
static struct sysent thread_self_trap_old_sysent;

static int host_self_trap_offset = NO_SYSCALL;
static struct sysent host_self_trap_old_sysent;

SYSCTL_INT(_mach_syscall, OID_AUTO, mach_reply_port, CTLFLAG_RD,
    &mach_reply_port_offset, 0,
    "Dynamically-allocated FreeBSD syscall number for mach_reply_port "
    "(-1 if registration failed)");

SYSCTL_INT(_mach_syscall, OID_AUTO, task_self_trap, CTLFLAG_RD,
    &task_self_trap_offset, 0,
    "Dynamically-allocated FreeBSD syscall number for task_self_trap "
    "(-1 if registration failed)");

SYSCTL_INT(_mach_syscall, OID_AUTO, thread_self_trap, CTLFLAG_RD,
    &thread_self_trap_offset, 0,
    "Dynamically-allocated FreeBSD syscall number for thread_self_trap "
    "(-1 if registration failed)");

SYSCTL_INT(_mach_syscall, OID_AUTO, host_self_trap, CTLFLAG_RD,
    &host_self_trap_offset, 0,
    "Dynamically-allocated FreeBSD syscall number for host_self_trap "
    "(-1 if registration failed)");

static void
wire_one(const char *name, int *offset, struct sysent *sy,
    struct sysent *old_sy)
{
	int error;

	error = kern_syscall_register(sysent, offset, sy, old_sy,
	    SY_THR_STATIC_KLD);
	if (error != 0) {
		printf("mach: syscall_register(%s) failed: %d\n", name, error);
		*offset = NO_SYSCALL;
		return;
	}
	printf("mach: %s registered at syscall %d\n", name, *offset);
}

static void
unwire_one(const char *name, int *offset, struct sysent *old_sy)
{
	int error;

	if (*offset == NO_SYSCALL)
		return;
	error = kern_syscall_deregister(sysent, *offset, old_sy);
	if (error != 0)
		printf("mach: syscall_deregister(%s) failed: %d\n", name, error);
}

static void
mach_syscall_wire_register(void *arg __unused)
{

	/*
	 * Pass `sysent` (the global FreeBSD syscall table) as the target.
	 * kern_syscall_register handles offset==NO_SYSCALL specially by
	 * finding a free slot and writing it back.
	 */
	wire_one("mach_reply_port", &mach_reply_port_offset,
	    &mach_reply_port_sysent, &mach_reply_port_old_sysent);
	wire_one("task_self_trap", &task_self_trap_offset,
	    &task_self_trap_sysent, &task_self_trap_old_sysent);
	wire_one("thread_self_trap", &thread_self_trap_offset,
	    &thread_self_trap_sysent, &thread_self_trap_old_sysent);
	wire_one("host_self_trap", &host_self_trap_offset,
	    &host_self_trap_sysent, &host_self_trap_old_sysent);
}

static void
mach_syscall_wire_deregister(void *arg __unused)
{

	unwire_one("host_self_trap", &host_self_trap_offset,
	    &host_self_trap_old_sysent);
	unwire_one("thread_self_trap", &thread_self_trap_offset,
	    &thread_self_trap_old_sysent);
	unwire_one("task_self_trap", &task_self_trap_offset,
	    &task_self_trap_old_sysent);
	unwire_one("mach_reply_port", &mach_reply_port_offset,
	    &mach_reply_port_old_sysent);
}

/*
 * Register AFTER SI_SUB_KLD so the IPC zones are up before any caller
 * could hit our handler. Deregister at the same priority on unload.
 */
SYSINIT(mach_syscall_wire, SI_SUB_INTRINSIC, SI_ORDER_ANY,
    mach_syscall_wire_register, NULL);
SYSUNINIT(mach_syscall_wire, SI_SUB_INTRINSIC, SI_ORDER_ANY,
    mach_syscall_wire_deregister, NULL);

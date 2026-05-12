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
 * Phase B / Tier 1 B scope: wire ONE syscall (sys_mach_reply_port).
 * No-args, returns a port name. Easiest to test from userland with
 * `syscall(num)`. Once this works, more syscalls can be added by
 * extending the static array below.
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
 * Forward-declare struct mach_reply_port_args. The full definition
 * comes from <sys/mach/_mach_sysproto.h> via mach_types.h's force-
 * include in compat_shim.h. We only need the prototype shape here
 * because we just take a pointer to sys_mach_reply_port; the args
 * struct isn't dereferenced from this file.
 */
struct mach_reply_port_args;

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

/*
 * Wrapper around sys_mach_reply_port that returns MACH_PORT_NULL (0)
 * when the calling process has no Mach state (p_machdata == NULL).
 * Without this guard, sys_mach_reply_port -> mach_reply_port ->
 * current_task()->itk_space deref page-faults for processes whose
 * mach_task_init didn't run.
 *
 * Phase B: this makes the wired syscall demonstration-callable from
 * any process. The "no Mach state" path returns the same value Apple's
 * API would return on no-resources (MACH_PORT_NULL=0). A future
 * Phase C lazy-init helper or libmach's mach_task_init can promote
 * the caller to a real Mach-aware state.
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

static struct sysent mach_reply_port_sysent = {
	.sy_narg	= 0,
	.sy_call	= (sy_call_t *)sys_mach_reply_port_guarded,
	.sy_auevent	= AUE_NULL,
	.sy_flags	= 0,
};

static int mach_reply_port_offset = NO_SYSCALL;
static struct sysent mach_reply_port_old_sysent;

SYSCTL_INT(_mach_syscall, OID_AUTO, mach_reply_port, CTLFLAG_RD,
    &mach_reply_port_offset, 0,
    "Dynamically-allocated FreeBSD syscall number for mach_reply_port "
    "(-1 if registration failed)");

static void
mach_syscall_wire_register(void *arg __unused)
{
	int error;

	/*
	 * Pass `sysent` (the global FreeBSD syscall table) as the target.
	 * kern_syscall_register handles offset==NO_SYSCALL specially by
	 * finding a free slot and writing it back.
	 */
	error = kern_syscall_register(sysent, &mach_reply_port_offset,
	    &mach_reply_port_sysent, &mach_reply_port_old_sysent,
	    SY_THR_STATIC_KLD);
	if (error != 0) {
		printf("mach: syscall_register(mach_reply_port) failed: %d\n",
		    error);
		mach_reply_port_offset = NO_SYSCALL;
		return;
	}
	printf("mach: sys_mach_reply_port registered at syscall %d\n",
	    mach_reply_port_offset);
}

static void
mach_syscall_wire_deregister(void *arg __unused)
{
	int error;

	if (mach_reply_port_offset == NO_SYSCALL)
		return;
	error = kern_syscall_deregister(sysent, mach_reply_port_offset,
	    &mach_reply_port_old_sysent);
	if (error != 0)
		printf("mach: syscall_deregister(mach_reply_port) failed: %d\n",
		    error);
}

/*
 * Register AFTER SI_SUB_KLD so the IPC zones are up before any caller
 * could hit our handler. Deregister at the same priority on unload.
 */
SYSINIT(mach_syscall_wire, SI_SUB_INTRINSIC, SI_ORDER_ANY,
    mach_syscall_wire_register, NULL);
SYSUNINIT(mach_syscall_wire, SI_SUB_INTRINSIC, SI_ORDER_ANY,
    mach_syscall_wire_deregister, NULL);

/*
 * compat_shim.h — out-of-tree adapters between ravynOS's Mach kernel
 * code and stock FreeBSD's struct proc / struct thread / syscall ABI.
 *
 * Force-included into every translation unit via the Makefile's
 * -include directive. Keeps the imported ravynOS sources unmodified;
 * all "stock-FreeBSD doesn't have this" deltas live here.
 */

#ifndef	_MACH_KMOD_COMPAT_SHIM_H_
#define	_MACH_KMOD_COMPAT_SHIM_H_

#ifdef _KERNEL

/*
 * This shim is force-included via the Makefile's -include directive,
 * which fires before any other include in the .c file. To avoid taking
 * any header dependency this early, the struct fields below use plain
 * C types instead of <sys/stdint.h> fixed-width typedefs. proc_info.c
 * passes these fields through to proc_info() which takes uint32_t /
 * uint64_t parameters — implicit integer conversion handles the gap
 * cleanly on amd64 where unsigned int / unsigned long long match the
 * underlying widths.
 */

/*
 * ravynOS adds p_machdata / td_machdata fields to struct proc / struct
 * thread to attach a Mach task and Mach thread to each FreeBSD proc/td.
 * Stock FreeBSD already provides exactly the right slot for this — the
 * emulation-data fields originally introduced for Linux compat:
 *
 *   struct proc::p_emuldata    -> Mach task_t  for this process
 *   struct thread::td_emuldata -> Mach thread_t for this kernel thread
 *
 * Aliasing instead of adding new fields keeps mach.ko a true out-of-
 * tree module: no kernel patches, no KBI bumps, no rebuild of the
 * kernel proper to install us. The cost is that mach.ko and the linux
 * compat module cannot both attach their state to the same proc — a
 * tradeoff we accept since neither runs Mach processes and Linux
 * processes in the same container.
 */
#define	p_machdata	p_emuldata
#define	td_machdata	td_emuldata

/*
 * Apple-shape syscalls referenced from proc_info.c. ravynOS generates
 * the struct *_args containers and sys_* prototypes from a syscalls
 * master file that we don't have on stock FreeBSD. We declare the
 * minimum surface here so the handlers compile; actual syscall
 * registration is a separate Phase-B follow-up (the handlers exist
 * but are not yet wired into a sysent table).
 */
struct __proc_info_args {
	int			callnum;
	int			pid;
	unsigned int		flavor;		/* matches uint32_t on amd64 */
	unsigned long long	arg;		/* matches uint64_t on amd64 */
	void			*buffer;	/* passed as void *addr to proc_info() */
	int			buffersize;	/* matches int32_t on amd64 */
};

struct __iopolicysys_args {
	int		dummy;	/* iopolicysys handler returns ENOSYS unconditionally */
};

struct thread;
int sys___proc_info(struct thread *td, struct __proc_info_args *uap);
int sys___iopolicysys(struct thread *td, struct __iopolicysys_args *uap);

/*
 * Apple Mach trap and BSD-extension syscalls referenced by
 * mach_module.c's SYSCALL_INIT_HELPER list. ravynOS auto-generates
 * their struct *_args, SYS_*, and SYS_AUE_* constants from a syscalls
 * master file we don't ship. Stub them here so mach_module.c's
 * osx_syscalls[] array initialization compiles.
 *
 * Each stub provides:
 *   - struct NAME_args { int _stub; }  (sizeof() works for sy_narg calc)
 *   - SYS_NAME           = -1          (FreeBSD NO_SYSCALL sentinel)
 *   - SYS_AUE_NAME       = 0           (FreeBSD AUE_NULL sentinel)
 *
 * Literals are used instead of <sys/sysent.h> / <bsm/audit_kevents.h>
 * because the compat shim is force-included before any other header in
 * the .c file; pulling in those headers this early risks ordering
 * problems with the kernel build's existing include chain. The
 * literals match the canonical definitions byte-for-byte.
 *
 * The handler bodies (sys_NAME) are defined in the ravynOS source
 * files; we only declare the prototype here so the address-taking in
 * SYSCALL_INIT_HELPER resolves. Phase B does NOT actually register
 * these via syscall_helper_register — see the wrapper macro below.
 */
#define	_MACH_KMOD_APPLE_SYSCALL_STUB(name)				\
	struct name ## _args { int _stub; };				\
	int sys_ ## name(struct thread *, struct name ## _args *);	\
	enum {								\
		SYS_ ## name = -1,		/* NO_SYSCALL */	\
		SYS_AUE_ ## name = 0		/* AUE_NULL */		\
	}

_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_vm_allocate_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_vm_deallocate_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_vm_protect_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_vm_map_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_allocate_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_destroy_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_deallocate_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_mod_refs_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_move_member_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_insert_right_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_insert_member_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_extract_member_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_construct_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_destruct_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_guard_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(_kernelrpc_mach_port_unguard_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(mach_reply_port);
_MACH_KMOD_APPLE_SYSCALL_STUB(thread_self_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(task_self_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(host_self_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(mach_msg_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(mach_msg_overwrite_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_signal_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_signal_all_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_signal_thread_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_wait_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_wait_signal_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_timedwait_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(semaphore_timedwait_signal_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(task_for_pid);
_MACH_KMOD_APPLE_SYSCALL_STUB(pid_for_task);
_MACH_KMOD_APPLE_SYSCALL_STUB(macx_swapon);
_MACH_KMOD_APPLE_SYSCALL_STUB(macx_swapoff);
_MACH_KMOD_APPLE_SYSCALL_STUB(macx_triggers);
_MACH_KMOD_APPLE_SYSCALL_STUB(macx_backing_store_suspend);
_MACH_KMOD_APPLE_SYSCALL_STUB(macx_backing_store_recovery);
_MACH_KMOD_APPLE_SYSCALL_STUB(swtch_pri);
_MACH_KMOD_APPLE_SYSCALL_STUB(swtch);
_MACH_KMOD_APPLE_SYSCALL_STUB(thread_switch);
_MACH_KMOD_APPLE_SYSCALL_STUB(clock_sleep_trap);
_MACH_KMOD_APPLE_SYSCALL_STUB(mach_timebase_info);
_MACH_KMOD_APPLE_SYSCALL_STUB(mach_wait_until);
_MACH_KMOD_APPLE_SYSCALL_STUB(mk_timer_create);
_MACH_KMOD_APPLE_SYSCALL_STUB(mk_timer_destroy);
_MACH_KMOD_APPLE_SYSCALL_STUB(mk_timer_arm);
_MACH_KMOD_APPLE_SYSCALL_STUB(mk_timer_cancel);

/*
 * Phase B: do not actually register these syscalls at module load.
 * Hijack syscall_helper_register so it's a no-op returning success.
 * The osx_syscalls[] array still gets built so the compile succeeds,
 * and mach_mod_init() proceeds to the kqueue_add_filteropts step
 * uneventfully. Real sysent registration is a separate Phase-B
 * follow-up that needs a syscalls master file and proper SYS_*
 * numbers per Apple's ABI — none of which is on Phase B's exit gate.
 */
#define	syscall_helper_register(table, flags)	(0)
#define	syscall_helper_unregister(table)	(0)

#endif /* _KERNEL */

#endif /* _MACH_KMOD_COMPAT_SHIM_H_ */

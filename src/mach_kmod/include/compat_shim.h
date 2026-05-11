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
/*
 * Real Mach syscall arg struct definitions are extracted from ravynOS's
 * generated sysproto.h and live at include/sys/mach/_mach_sysproto.h.
 * They use Apple Mach types (mach_msg_header_t, mach_port_name_t, etc.)
 * which need <sys/mach/mach_types.h> + friends visible first. Pull them
 * in here so the structs parse correctly; subsequent .c files including
 * the same headers see the idempotent guards.
 */
#include <sys/mach/_mach_sysproto.h>

struct thread;
int sys___proc_info(struct thread *td, struct __proc_info_args *uap);
int sys___iopolicysys(struct thread *td, struct __iopolicysys_args *uap);

/*
 * SYS_/SYS_AUE_ constants for the two syscalls whose struct *_args
 * needed real field definitions (proc_info.c reads them). The other
 * 47 syscalls get their constants via the _MACH_KMOD_APPLE_SYSCALL_STUB
 * macro below.
 */
enum {
	SYS___proc_info		= -1,	/* NO_SYSCALL */
	SYS_AUE___proc_info	= 0,	/* AUE_NULL */
	SYS___iopolicysys	= -1,	/* NO_SYSCALL */
	SYS_AUE___iopolicysys	= 0,	/* AUE_NULL */
};

/*
 * EVFILT_MACHPORT — Apple kqueue filter ID for Mach port events.
 * ravynOS patches FreeBSD's <sys/event.h> to add this. Stock FreeBSD
 * reserves filter IDs -1 .. -EVFILT_SYSCOUNT (currently -15) for
 * built-in filters; -16 is outside that range and safe to claim for
 * a kmod-registered filter via kqueue_add_filteropts(EVFILT_MACHPORT,
 * &machport_filtops).
 */
#define	EVFILT_MACHPORT		(-16)

/*
 * Apple-shape file-descriptor type tags ravynOS adds to FreeBSD headers
 * so Mach ports can be wrapped as file descriptors visible to procstat,
 * fstat, etc. Stock FreeBSD has none of these defined. Values match
 * ravynOS's sys/sys/stat.h, sys/sys/file.h, sys/sys/user.h.
 */
#define	S_IFPORT	0150000		/* mach port */
#define	S_IFPSET	0160000		/* mach portset */
#define	DTYPE_MACH_IPC	32		/* port or portset */
#define	KF_TYPE_PORT	253
#define	KF_TYPE_PORTSET	254

/*
 * ipc_entry.c calls audit_sysclose, which IS in stock FreeBSD with the
 * same signature — just declared in <security/audit/audit.h>, which
 * ipc_entry.c doesn't include directly. Pull it in here so the
 * prototype is visible.
 */
#include <security/audit/audit.h>

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
/*
 * For each Apple-shape syscall, declare the prototype (the struct is
 * already defined by _mach_sysproto.h above) and the SYS_/SYS_AUE_
 * constants that mach_module.c references via SYSCALL_INIT_HELPER.
 * Constants are NO_SYSCALL (-1) so syscall_helper_register dynamically
 * allocates slots, and AUE_NULL (0) so no audit event is emitted.
 */
#define	_MACH_KMOD_APPLE_SYSCALL_STUB(name)				\
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
 * Phase B: syscalls register dynamically. Each entry's syscall_no field
 * is set to NO_SYSCALL (-1) via the stubs above, which tells
 * syscall_helper_register / kern_syscall_register to allocate a free
 * slot at runtime instead of using a fixed number. We don't macro-
 * hijack syscall_helper_register here — that would clash with the
 * function declaration in <sys/sysent.h>. If the runtime dynamic
 * allocation fails for some entries, we'll see that in the boot smoke
 * test and iterate.
 */

/*
 * Three VM functions ravynOS's mach_vm.c calls are static inline in
 * stock FreeBSD's vm/vm_map.c — not exported to modules. Provide
 * replacements as macros that expand at the call site, where <vm/vm_map.h>
 * is already in scope (mach_vm.c #include's it). Doing this with
 * static-inline definitions in the shim doesn't work because
 * <vm/vm_map.h> can't be included safely from a force-included header:
 * its struct vm_map references pmap_t (from <machine/pmap.h>), which
 * the kernel build expects to be brought in by the .c file before
 * <vm/vm_map.h>, not from a force-include header.
 *
 * - vm_map_entry_pred: macro version of the static inline in vm_map.c,
 *   using GCC statement-expression. Uses only public struct
 *   vm_map_entry fields (left/right/start).
 *
 * - vm_map_clip_start / vm_map_clip_end: stub macros that evaluate to 0
 *   (KERN_SUCCESS). The only call site in mach_vm.c is inside an
 *   if (src_destroy) path of vm_map_copy_overwrite(), which Phase B's
 *   smoke test doesn't exercise. When the destroy-copy path is needed
 *   for real (later phase), these stubs need real implementations.
 */
#define	vm_map_entry_pred(entry_) __extension__ ({			\
	vm_map_entry_t _prior = (entry_)->left;				\
	if (_prior->right->start < (entry_)->start) {			\
		do							\
			_prior = _prior->right;				\
		while (_prior->right != (entry_));			\
	}								\
	_prior;								\
})

#define	vm_map_clip_start(map_, entry_, startaddr_)	((void)(map_), (void)(entry_), (void)(startaddr_), 0)
#define	vm_map_clip_end(map_, entry_, endaddr_)		((void)(map_), (void)(entry_), (void)(endaddr_), 0)

#endif /* _KERNEL */

#endif /* _MACH_KMOD_COMPAT_SHIM_H_ */

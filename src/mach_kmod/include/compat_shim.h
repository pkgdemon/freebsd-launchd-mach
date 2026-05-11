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
 * which fires before any other include in the .c file. The fixed-width
 * integer types we need below come from <sys/types.h>; pull it in here
 * so the typedefs are visible when the shim's own declarations are
 * parsed.
 */
#include <sys/types.h>

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
	int		callnum;
	int		pid;
	uint32_t	flavor;
	uint64_t	arg;
	uint64_t	buffer;
	int32_t		buffersize;
};

struct __iopolicysys_args {
	int		dummy;	/* iopolicysys handler returns ENOSYS unconditionally */
};

struct thread;
int sys___proc_info(struct thread *td, struct __proc_info_args *uap);
int sys___iopolicysys(struct thread *td, struct __iopolicysys_args *uap);

#endif /* _KERNEL */

#endif /* _MACH_KMOD_COMPAT_SHIM_H_ */

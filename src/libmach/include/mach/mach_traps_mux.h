/*
 * mach/mach_traps_mux.h — multiplexer ABI for Mach kernel traps.
 *
 * Background. FreeBSD's syscalls.master reserves exactly 10 lkmnosys
 * slots (210-219) for KLD-allocated syscalls. freebsd-launchd-mach
 * fills all 10 with the foundational Mach trap surface; the eventual
 * launchd+configd+notifyd+asl port wants 20-30 more Mach traps. To
 * fit them without patching the FreeBSD base kernel, we route all
 * NEW Mach traps through a single multiplexer syscall: one FreeBSD
 * slot at a claimed RESERVED-range index (currently slot 91), with
 * an op-number selecting which Mach trap to invoke. Approximates
 * Apple's mach_trap_table architecture one level later in the stack
 * (C handler instead of asm trampoline).
 *
 * Wire contract.
 *   long syscall(MACH_TRAP_MUX_SYSCALL, op, a1, a2, a3, a4, a5);
 *                ^^^^^^^^^^^^^^^^^^^^^  ^^  ^^^^^^^^^^^^^^^^^^
 *                resolved via sysctl    op  up to 5 user args
 *
 * libc syscall() carries up to 6 register args on amd64 (rdi-r9).
 * The op-number uses one; the remaining five are passed through to
 * the underlying Mach trap. Traps needing 6+ args (mach_msg_trap,
 * mach_msg_overwrite_trap) stay in dedicated FreeBSD slots — they
 * don't go through the multiplexer.
 *
 * Op-number stability. Op numbers are stable across releases (unlike
 * the dynamically-assigned FreeBSD syscall slot). Add new ops at the
 * end; never renumber.
 */
#ifndef _MACH_MACH_TRAPS_MUX_H_
#define _MACH_MACH_TRAPS_MUX_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Multiplexer op numbers.
 *
 *   0 — reserved sentinel ("invalid op"); kernel rejects with
 *       KERN_INVALID_ARGUMENT.
 *   1 — host_set_special_port(host, which, port)
 *
 * New ops append at the end. The Mach traps currently wired in
 * dedicated FreeBSD slots (mach_reply_port, task_self_trap, ...,
 * mach_msg_trap) do NOT have op numbers — they keep their dedicated
 * slots for stability and (in the case of mach_msg_trap) ABI
 * reasons.
 */
#define MACH_TRAP_OP_INVALID			0
#define MACH_TRAP_OP_HOST_SET_SPECIAL_PORT	1

#define MACH_TRAP_OP_MAX			1

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_TRAPS_MUX_H_ */

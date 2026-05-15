/*
 * mach/exception.h — Mach exception type / mask / behavior constants
 * plus the task_set_exception_ports() / host_set_exception_ports()
 * RPC declarations. launchd-842's core.c arms EXC_MASK_CRASH |
 * EXC_MASK_GUARD | EXC_MASK_RESOURCE with EXCEPTION_STATE_IDENTITY |
 * MACH_EXCEPTION_CODES so it can field crash exceptions for the jobs
 * it supervises.
 *
 * Values match Apple's <mach/exception_types.h>. The catch_* server
 * side of the protocol is MIG-generated from mach_exc.defs; this
 * header is the client/setup side.
 */
#ifndef _MACH_EXCEPTION_H_
#define _MACH_EXCEPTION_H_

#include <mach/mach_types.h>	/* task_t, host_priv_t, mach_port_t, kern_return_t */
#include <mach/thread_status.h>	/* thread_state_flavor_t */

#include <stdint.h>
#include <mach/std_types.h>	/* exception_type_t, mach_exception_data_t (shared) */

typedef int		exception_behavior_t;
typedef unsigned int	exception_mask_t;

/* Exception types. */
#define EXC_BAD_ACCESS		1
#define EXC_BAD_INSTRUCTION	2
#define EXC_ARITHMETIC		3
#define EXC_EMULATION		4
#define EXC_SOFTWARE		5
#define EXC_BREAKPOINT		6
#define EXC_SYSCALL		7
#define EXC_MACH_SYSCALL	8
#define EXC_RPC_ALERT		9
#define EXC_CRASH		10
#define EXC_RESOURCE		11
#define EXC_GUARD		12
#define EXC_CORPSE_NOTIFY	13

/* Exception masks (1 << type). */
#define EXC_MASK_BAD_ACCESS	(1u << EXC_BAD_ACCESS)
#define EXC_MASK_BAD_INSTRUCTION (1u << EXC_BAD_INSTRUCTION)
#define EXC_MASK_ARITHMETIC	(1u << EXC_ARITHMETIC)
#define EXC_MASK_EMULATION	(1u << EXC_EMULATION)
#define EXC_MASK_SOFTWARE	(1u << EXC_SOFTWARE)
#define EXC_MASK_BREAKPOINT	(1u << EXC_BREAKPOINT)
#define EXC_MASK_SYSCALL	(1u << EXC_SYSCALL)
#define EXC_MASK_MACH_SYSCALL	(1u << EXC_MACH_SYSCALL)
#define EXC_MASK_RPC_ALERT	(1u << EXC_RPC_ALERT)
#define EXC_MASK_CRASH		(1u << EXC_CRASH)
#define EXC_MASK_RESOURCE	(1u << EXC_RESOURCE)
#define EXC_MASK_GUARD		(1u << EXC_GUARD)
#define EXC_MASK_CORPSE_NOTIFY	(1u << EXC_CORPSE_NOTIFY)

/* Exception behaviors. */
#define EXCEPTION_DEFAULT		1
#define EXCEPTION_STATE			2
#define EXCEPTION_STATE_IDENTITY	3
#define MACH_EXCEPTION_CODES		0x80000000

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t task_set_exception_ports(task_t task,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t behavior, thread_state_flavor_t new_flavor);

kern_return_t host_set_exception_ports(host_priv_t host_priv,
    exception_mask_t exception_mask, mach_port_t new_port,
    exception_behavior_t behavior, thread_state_flavor_t new_flavor);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_EXCEPTION_H_ */

/*
 * mach/thread_status.h — thread-state type vocabulary.
 *
 * mach_types.defs imports this for the thread_state_t type used by
 * the thread_get_state / thread_set_state RPC family. launchd-842's
 * job interface doesn't actually transfer thread state, but the
 * import is unconditional in mach_types.defs, so the MIG-generated
 * job.h #includes it — hence this minimal header.
 *
 * We provide just the type vocabulary (thread_state_t,
 * thread_state_flavor_t, THREAD_STATE_MAX) — not the per-architecture
 * x86_THREAD_STATE* / ARM_THREAD_STATE* register-block structs, which
 * nothing in the launchd MIG surface references.
 */
#ifndef _MACH_THREAD_STATUS_H_
#define _MACH_THREAD_STATUS_H_

#include <mach/std_types.h>	/* natural_t */

/* Upper bound on a thread-state blob, in natural_t units. Apple's
 * value is machine-dependent; 224 covers x86_64's largest flavor.
 * mach_types.defs only references it inside `#ifndef THREAD_STATE_MAX`
 * so this is just the fallback. */
#ifndef THREAD_STATE_MAX
#define THREAD_STATE_MAX	224
#endif

/* A thread-state blob is a variable-length natural_t array; the
 * flavor selects which register block it holds. */
typedef natural_t		*thread_state_t;
typedef natural_t		thread_state_data_t[THREAD_STATE_MAX];
typedef int			thread_state_flavor_t;
typedef thread_state_flavor_t	*thread_state_flavor_array_t;

/*
 * Per-arch flavor constants. launchd-842's core.c picks one based on
 * the host architecture to pass to task_set_exception_ports(). On
 * Apple they live in <mach/i386/thread_status.h> and
 * <mach/arm/thread_status.h>; here the flavor numbers are enough —
 * we don't transfer register state.
 */
#define x86_THREAD_STATE		7
#define x86_THREAD_STATE32		1
#define x86_THREAD_STATE64		4
#define ARM_THREAD_STATE		1
#define ARM_THREAD_STATE64		6

#endif /* !_MACH_THREAD_STATUS_H_ */

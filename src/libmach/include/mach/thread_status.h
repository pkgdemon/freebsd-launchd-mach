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

#endif /* !_MACH_THREAD_STATUS_H_ */

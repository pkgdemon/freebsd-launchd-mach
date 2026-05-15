/*
 * mach/mach_traps.h — public libmach API for the no-arg Mach trap family.
 *
 * Each function corresponds to a syscall registered by mach.ko at module
 * load via kern_syscall_register. The FreeBSD syscall number is dynamic —
 * libmach resolves it from sysctl mach.syscall.<name> on first call and
 * caches the result.
 *
 * If mach.ko is not loaded or the syscall isn't registered, these
 * functions return MACH_PORT_NULL.
 */
#ifndef _MACH_MACH_TRAPS_H_
#define _MACH_MACH_TRAPS_H_

#include <mach/kern_return.h>	/* kern_return_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int mach_port_name_t;

#define MACH_PORT_NULL ((mach_port_name_t)0)

mach_port_name_t mach_reply_port(void);
mach_port_name_t mach_task_self(void);
mach_port_name_t mach_thread_self(void);
mach_port_name_t mach_host_self(void);

/*
 * pid_for_task() — map a task port back to its owning pid. Apple-
 * canonical; lives alongside the other task-introspection traps.
 */
kern_return_t pid_for_task(mach_port_name_t task, int *pid);

/*
 * task_self_trap() — raw-syscall name for "give me my task port".
 * Some Apple source uses this rather than mach_task_self().
 */
mach_port_name_t task_self_trap(void);

/*
 * task_name_for_pid() — open a task-name port (introspection-only)
 * for a given pid. Stubbed; returns KERN_FAILURE.
 */
kern_return_t task_name_for_pid(mach_port_name_t target_task,
    int pid, mach_port_name_t *t);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_TRAPS_H_ */

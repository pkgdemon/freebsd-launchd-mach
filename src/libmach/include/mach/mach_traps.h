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

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int mach_port_name_t;

#define MACH_PORT_NULL ((mach_port_name_t)0)

mach_port_name_t mach_reply_port(void);
mach_port_name_t mach_task_self(void);
mach_port_name_t mach_thread_self(void);
mach_port_name_t mach_host_self(void);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_TRAPS_H_ */

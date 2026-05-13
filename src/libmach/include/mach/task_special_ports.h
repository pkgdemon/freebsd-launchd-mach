/*
 * mach/task_special_ports.h — userland API for the per-task special-port
 * slots (TASK_KERNEL_PORT, TASK_HOST_PORT, TASK_BOOTSTRAP_PORT, ...).
 *
 * Wraps two mach.ko-registered syscalls (task_get_special_port,
 * task_set_special_port). On Apple these go through MIG over the task
 * port; we expose them as direct syscalls instead. The Apple-canonical
 * task_get_bootstrap_port / task_set_bootstrap_port etc. macros are
 * preserved so Apple-source consumers compile unchanged.
 *
 * Returns KERN_RESOURCE_SHORTAGE if mach.ko is not loaded.
 */
#ifndef _MACH_TASK_SPECIAL_PORTS_H_
#define _MACH_TASK_SPECIAL_PORTS_H_

#include <mach/mach_traps.h>	/* mach_port_name_t, MACH_PORT_NULL */
#include <mach/mach_port.h>	/* kern_return_t */
#include <mach/message.h>	/* mach_port_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef int task_special_port_t;

/* Selectors for which special-port slot to get/set. Matches
 * <sys/mach/task_special_ports.h> on the kernel side. */
#define TASK_KERNEL_PORT		1
#define TASK_HOST_PORT			2
#define TASK_NAME_PORT			3
#define TASK_BOOTSTRAP_PORT		4
#define TASK_SEATBELT_PORT		7
#define TASK_ACCESS_PORT		9
#define TASK_DEBUG_CONTROL_PORT		17

kern_return_t task_get_special_port(mach_port_name_t task,
    int which, mach_port_name_t *port);

kern_return_t task_set_special_port(mach_port_name_t task,
    int which, mach_port_t port);

/* Apple-canonical convenience macros. Same shape as
 * <sys/mach/task_special_ports.h>'s kernel-side versions. */
#define task_get_bootstrap_port(task, port) \
		(task_get_special_port((task), TASK_BOOTSTRAP_PORT, (port)))

#define task_set_bootstrap_port(task, port) \
		(task_set_special_port((task), TASK_BOOTSTRAP_PORT, (port)))

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_TASK_SPECIAL_PORTS_H_ */

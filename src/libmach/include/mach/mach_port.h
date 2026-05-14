/*
 * mach/mach_port.h — userland API for the port-management Mach traps.
 *
 * Wraps the three syscalls registered by mach.ko at module load:
 * mach_port_allocate (3 args), mach_port_deallocate (2 args),
 * mach_port_insert_right (4 args). Numbers are dynamic — libsystem_kernel
 * resolves them via `sysctl mach.syscall.<name>` on first call.
 *
 * Consumers: ravynOS-fork libxpc and anyone else that needs to allocate
 * Mach ports beyond the four-trap family (task/thread/host/reply).
 *
 * If mach.ko is not loaded these functions return KERN_RESOURCE_SHORTAGE
 * — matching Apple's "couldn't allocate" code in the standard return set.
 */
#ifndef _MACH_MACH_PORT_H_
#define _MACH_MACH_PORT_H_

#include <mach/mach_traps.h>	/* mach_port_name_t, MACH_PORT_NULL */
#include <mach/message.h>	/* mach_port_t */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _KERN_RETURN_T_DEFINED
#define _KERN_RETURN_T_DEFINED
typedef int kern_return_t;
#endif
typedef unsigned int mach_port_right_t;
/* mach_msg_type_name_t is now declared in <mach/message.h> so that
 * <mach/message.h> can use it standalone (mach_msg_port_descriptor_t
 * needs it) without pulling in mach_port.h and creating a cycle. */

/* Standard kern_return_t codes we care about; full set in
 * <sys/mach/kern_return.h> on the kernel side. */
#define KERN_SUCCESS              0
#define KERN_INVALID_ARGUMENT     4
#define KERN_FAILURE              5
#define KERN_RESOURCE_SHORTAGE    6
#define KERN_INVALID_TASK         16	/* matches <sys/mach/kern_return.h> */

/* mach_port_right_t values (matches <sys/mach/port.h>). */
#define MACH_PORT_RIGHT_SEND        ((mach_port_right_t)0)
#define MACH_PORT_RIGHT_RECEIVE     ((mach_port_right_t)1)
#define MACH_PORT_RIGHT_SEND_ONCE   ((mach_port_right_t)2)
#define MACH_PORT_RIGHT_PORT_SET    ((mach_port_right_t)3)
#define MACH_PORT_RIGHT_DEAD_NAME   ((mach_port_right_t)4)

/* MACH_MSG_TYPE_* port-right dispositions now live in their Apple-
 * canonical home, <mach/message.h> (which this header #includes
 * above). The full set — MOVE_*/COPY_*/MAKE_*, PORT_* aliases,
 * POLYMORPHIC — is declared there once; declaring them here too
 * caused duplicate-definition breakage when migcom pulled in only
 * <mach/message.h>. */

kern_return_t mach_port_allocate(mach_port_name_t task,
    mach_port_right_t right, mach_port_name_t *name);

kern_return_t mach_port_deallocate(mach_port_name_t task,
    mach_port_name_t name);

kern_return_t mach_port_insert_right(mach_port_name_t task,
    mach_port_name_t name, mach_port_t poly,
    mach_msg_type_name_t polyPoly);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_PORT_H_ */

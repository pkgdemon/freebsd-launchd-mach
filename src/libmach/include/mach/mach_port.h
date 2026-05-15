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
#define KERN_INVALID_RIGHT        17
#define KERN_INVALID_NAME         15
#define KERN_INVALID_VALUE        18
#define KERN_NO_SPACE             3

/* mach_port_right_t values (matches <sys/mach/port.h>). */
#define MACH_PORT_RIGHT_SEND        ((mach_port_right_t)0)
#define MACH_PORT_RIGHT_RECEIVE     ((mach_port_right_t)1)
#define MACH_PORT_RIGHT_SEND_ONCE   ((mach_port_right_t)2)
#define MACH_PORT_RIGHT_PORT_SET    ((mach_port_right_t)3)
#define MACH_PORT_RIGHT_DEAD_NAME   ((mach_port_right_t)4)

/*
 * MACH_MSG_TYPE_* port-right dispositions now live in their Apple-
 * canonical home, <mach/message.h> (which this header #includes
 * above). The full set -- the MOVE / COPY / MAKE family, the PORT_
 * abstract aliases, and POLYMORPHIC -- is declared there once.
 * Declaring them here too caused duplicate-definition breakage when
 * migcom pulled in only <mach/message.h>.
 */

kern_return_t mach_port_allocate(mach_port_name_t task,
    mach_port_right_t right, mach_port_name_t *name);

kern_return_t mach_port_deallocate(mach_port_name_t task,
    mach_port_name_t name);

kern_return_t mach_port_insert_right(mach_port_name_t task,
    mach_port_name_t name, mach_port_t poly,
    mach_msg_type_name_t polyPoly);

/*
 * mach_port_get_attributes() / set_attributes() flavor codes + their
 * companion structs. launchd-842's core.c calls
 * mach_port_set_attributes(.., MACH_PORT_LIMITS_INFO, ..) to raise a
 * receive port's queue depth. Apple-canonical values.
 */
typedef integer_t	*mach_port_info_t;
typedef int		mach_port_flavor_t;

#define MACH_PORT_LIMITS_INFO		1
#define MACH_PORT_RECEIVE_STATUS	2
#define MACH_PORT_DNREQUESTS_SIZE	3

/*
 * Port-vocabulary typedefs that live in <mach/port.h> on Apple, but
 * port.h includes us — so re-declaring them here behind shared
 * guards avoids a cycle while letting mach_port.h's structs (below)
 * compile standalone.
 */
#ifndef _MACH_PORT_MSGCOUNT_T_
#define _MACH_PORT_MSGCOUNT_T_
typedef natural_t mach_port_msgcount_t;
#endif
#ifndef _MACH_PORT_RIGHTS_T_
#define _MACH_PORT_RIGHTS_T_
typedef natural_t mach_port_rights_t;
#endif
#ifndef _MACH_PORT_SEQNO_T_
#define _MACH_PORT_SEQNO_T_
typedef natural_t mach_port_seqno_t;
#endif
#ifndef _MACH_PORT_MSCOUNT_T_
#define _MACH_PORT_MSCOUNT_T_
typedef natural_t mach_port_mscount_t;
#endif

struct mach_port_limits {
	mach_port_msgcount_t	mpl_qlimit;	/* port queue depth */
};
typedef struct mach_port_limits	mach_port_limits_t;

/*
 * struct mach_port_status — receive-side port introspection. Apple's
 * canonical layout; launchd-842 reads mps_msgcount to gauge queue
 * pressure on its bootstrap ports.
 */
struct mach_port_status {
	mach_port_rights_t	mps_pset;
	mach_port_seqno_t	mps_seqno;
	mach_port_mscount_t	mps_mscount;
	mach_port_msgcount_t	mps_qlimit;
	mach_port_msgcount_t	mps_msgcount;
	mach_port_rights_t	mps_sorights;
	boolean_t		mps_srights;
	boolean_t		mps_pdrequest;
	boolean_t		mps_nsrequest;
	natural_t		mps_flags;
};
typedef struct mach_port_status	mach_port_status_t;

#define MACH_PORT_RECEIVE_STATUS_COUNT \
	((mach_msg_type_number_t)(sizeof(mach_port_status_t) / sizeof(natural_t)))

#define MACH_PORT_LIMITS_INFO_COUNT \
	((mach_msg_type_number_t)(sizeof(mach_port_limits_t) / sizeof(integer_t)))

/*
 * mach_port_construct() option bits. MACH_PORT_TEMPOWNER is the only
 * one launchd-842 references — it asks the kernel to transfer
 * receive-right ownership on the next send, used for one-shot reply
 * ports handed across task boundaries.
 */
#define MACH_PORT_TEMPOWNER	0x01

kern_return_t mach_port_set_attributes(mach_port_name_t task,
    mach_port_name_t name, mach_port_flavor_t flavor,
    mach_port_info_t info, mach_msg_type_number_t infoCnt);

kern_return_t mach_port_get_attributes(mach_port_name_t task,
    mach_port_name_t name, mach_port_flavor_t flavor,
    mach_port_info_t info, mach_msg_type_number_t *infoCnt);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_PORT_H_ */

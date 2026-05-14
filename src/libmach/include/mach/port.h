/*
 * mach/port.h — core Mach port type vocabulary.
 *
 * Apple's <mach/port.h> is the base header defining the port name /
 * port right / port type vocabulary; <mach/mach_port.h> (the
 * port-management RPC layer) includes it. Several Apple-source
 * headers — notably libdispatch's <dispatch/source.h> — pull in
 * <mach/port.h> directly for just the mach_port_t type.
 *
 * Our split keeps mach_port_name_t / MACH_PORT_NULL in
 * <mach/mach_traps.h> and mach_port_t in <mach/message.h>; this
 * header re-exports both and adds the remaining base-vocabulary
 * constants (MACH_PORT_DEAD, the MACH_PORT_TYPE_* set) so a
 * consumer that includes only <mach/port.h> gets the full base
 * surface, matching Apple.
 */
#ifndef _MACH_PORT_H_
#define _MACH_PORT_H_

#include <stdint.h>
#include <mach/mach_traps.h>	/* mach_port_name_t, MACH_PORT_NULL */
#include <mach/message.h>	/* mach_port_t */
#include <mach/mach_port.h>	/* mach_port_right_t, MACH_PORT_RIGHT_* */

#ifndef MACH_PORT_DEAD
#define MACH_PORT_DEAD		((mach_port_name_t)~0)
#endif

#define MACH_PORT_VALID(name) \
	(((name) != MACH_PORT_NULL) && ((name) != MACH_PORT_DEAD))

/* mach_port_type_t — a bitmask describing which rights a name holds.
 * The MACH_PORT_RIGHT_* values come from <mach/mach_port.h>. */
typedef natural_t mach_port_type_t;
typedef mach_port_type_t *mach_port_type_array_t;

#define MACH_PORT_TYPE(right)		((mach_port_type_t)(1 << ((right) + 16)))
#define MACH_PORT_TYPE_NONE		((mach_port_type_t)0)
#define MACH_PORT_TYPE_SEND		MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND)
#define MACH_PORT_TYPE_RECEIVE		MACH_PORT_TYPE(MACH_PORT_RIGHT_RECEIVE)
#define MACH_PORT_TYPE_SEND_ONCE	MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_SET		MACH_PORT_TYPE(MACH_PORT_RIGHT_PORT_SET)
#define MACH_PORT_TYPE_DEAD_NAME	MACH_PORT_TYPE(MACH_PORT_RIGHT_DEAD_NAME)

typedef natural_t mach_port_urefs_t;
typedef integer_t mach_port_delta_t;
typedef natural_t mach_port_seqno_t;
typedef natural_t mach_port_mscount_t;
typedef natural_t mach_port_msgcount_t;
typedef natural_t mach_port_rights_t;

#endif /* !_MACH_PORT_H_ */

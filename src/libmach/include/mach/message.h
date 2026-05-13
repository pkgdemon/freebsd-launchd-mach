/*
 * mach/message.h — minimal userland API for mach_msg.
 *
 * Just enough surface to call mach_msg() through libmach. Type names
 * match Apple's mach/message.h so code can be written portably; the
 * full set of message-descriptor / kernel-internal types from
 * ravynOS's kernel-side header is intentionally not exposed here.
 */
#ifndef _MACH_MESSAGE_H_
#define _MACH_MESSAGE_H_

#include <mach/mach_traps.h>	/* mach_port_name_t, MACH_PORT_NULL */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int mach_msg_bits_t;
typedef unsigned int mach_msg_size_t;
typedef int          mach_msg_id_t;
typedef unsigned int mach_msg_option_t;
typedef unsigned int mach_msg_timeout_t;
typedef int          mach_msg_return_t;
typedef mach_port_name_t mach_port_t;

typedef struct {
	mach_msg_bits_t   msgh_bits;
	mach_msg_size_t   msgh_size;
	mach_port_t       msgh_remote_port;
	mach_port_t       msgh_local_port;
	mach_port_name_t  msgh_voucher_port;
	mach_msg_id_t     msgh_id;
} mach_msg_header_t;

/* Options bitmask values (matches sys/mach/message.h). */
#define MACH_MSG_OPTION_NONE	0x00000000
#define MACH_SEND_MSG		0x00000001
#define MACH_RCV_MSG		0x00000002
#define MACH_RCV_LARGE		0x00000004
#define MACH_SEND_TIMEOUT	0x00000010
#define MACH_RCV_TIMEOUT	0x00000100

/* Return codes we care about. Full set in sys/mach/message.h. */
#define MACH_MSG_SUCCESS	0x00000000
#define MACH_RCV_TIMED_OUT	0x10004003
#define MACH_RCV_TOO_LARGE	0x10004004

/*
 * msgh_bits port-disposition values (subset of MACH_MSG_TYPE_*). 21 =
 * MAKE_SEND_ONCE — sender constructs a send-once right from a receive
 * right it owns, which is the path used to send a message to one's own
 * receive port (e.g., self-test wakeups for the libdispatch Mach RECV
 * polling thread).
 */
#define MACH_MSG_TYPE_MAKE_SEND_ONCE	21

/*
 * MACH_MSGH_BITS(remote, local) — pack a remote-port and local-port
 * disposition into msgh_bits. Matches sys/mach/message.h.
 */
#define MACH_MSGH_BITS(remote, local) \
	((remote) | ((local) << 8))

mach_msg_return_t mach_msg(
    mach_msg_header_t *msg,
    mach_msg_option_t  option,
    mach_msg_size_t    send_size,
    mach_msg_size_t    rcv_size,
    mach_port_name_t   rcv_name,
    mach_msg_timeout_t timeout,
    mach_port_name_t   notify);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MESSAGE_H_ */

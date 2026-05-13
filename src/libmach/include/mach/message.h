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

#include <stdint.h>		/* uint16_t for mach_msg_port_descriptor_t */
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
typedef unsigned char mach_msg_type_name_t;	/* port-right disposition */

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

/*
 * MACH_MSGH_BITS_COMPLEX — set on msgh_bits to signal that the message
 * body has descriptors (port references / OOL memory) after the header.
 * The kernel translates port descriptors between sender / receiver IPC
 * spaces on this flag.
 */
#define MACH_MSGH_BITS_COMPLEX		0x80000000U

/*
 * Descriptor types (msgh_descriptor->type). PORT_DESCRIPTOR carries a
 * port reference; OOL_* carry out-of-line memory (not used yet).
 */
typedef unsigned char mach_msg_descriptor_type_t;
#define MACH_MSG_PORT_DESCRIPTOR		0
#define MACH_MSG_OOL_DESCRIPTOR			1
#define MACH_MSG_OOL_PORTS_DESCRIPTOR		2
#define MACH_MSG_OOL_VOLATILE_DESCRIPTOR	3

/*
 * Complex-message layout: header + body + descriptors + payload.
 *
 *   - mach_msg_body_t holds descriptor_count.
 *   - Each descriptor (mach_msg_port_descriptor_t or one of the OOL
 *     variants) follows immediately.
 *   - Payload (user-defined fields) follows the descriptor array.
 *
 * #pragma pack(4) on the descriptor types matches the kernel-side
 * layout in <sys/mach/message.h>; without it 64-bit alignment would
 * insert a 4-byte gap after `name` and break wire compatibility.
 *
 * Wire layout is unambiguously 12 bytes per port descriptor. The
 * kernel-side header uses bitfields (pad2:16 / disposition:8 /
 * type:8) which would also be 4 bytes IF the compiler packs them
 * into a single storage unit — but clang on FreeBSD is happy to
 * give each bitfield its own unsigned-int slot when the underlying
 * types differ, producing a 16-byte struct on the wire. To avoid
 * that ambiguity we encode the same on-wire layout with plain
 * uint16_t / uint8_t fields. Apple-source consumers using
 * .disposition / .type still compile (the types collapse to the
 * same unsigned char on both sides).
 */
#pragma pack(4)

typedef struct {
	mach_msg_size_t		msgh_descriptor_count;
} mach_msg_body_t;

typedef struct {
	mach_port_t			name;		/* 4 */
	mach_msg_size_t			pad1;		/* 4 */
	uint16_t			pad2;		/* 2 */
	mach_msg_type_name_t		disposition;	/* 1 — MACH_MSG_TYPE_* */
	mach_msg_descriptor_type_t	type;		/* 1 — MACH_MSG_PORT_DESCRIPTOR */
} mach_msg_port_descriptor_t;	/* total: 12 bytes */

#pragma pack()

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

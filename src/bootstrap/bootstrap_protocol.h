/*
 * bootstrap_protocol.h — internal message format between libbootstrap
 * client functions and the bootstrap server. Shared by both sides.
 *
 * Hand-rolled message-ID dispatch (no MIG). Each request type has a
 * distinct msgh_id; the reply uses request_id + REPLY_OFFSET.
 *
 * Wire model (Phase G1 — single-task only):
 *   - Request: header + name string (fixed-length, null-terminated).
 *   - Reply: header + kern_return_t result + mach_port_name_t port.
 *   - Port-name field transports a uint32_t directly; in a single
 *     task that's a valid port name in the receiver's view too.
 *
 * Phase G2 will upgrade to complex messages with port descriptors so
 * that cross-task port-right transfer works. The protocol header
 * stays the same — only the wire encoding of the port field changes.
 */
#ifndef _FREEBSD_LAUNCHD_MACH_BOOTSTRAP_PROTOCOL_H_
#define _FREEBSD_LAUNCHD_MACH_BOOTSTRAP_PROTOCOL_H_

#include <stdint.h>
#include <mach/message.h>
#include <mach/mach_port.h>		/* kern_return_t */

/*
 * Request / reply IDs. Base picks something recognizable in
 * `dispatch / kdump` traces ("BST\0"). Replies are request + 0x100
 * so they're trivially distinguishable.
 */
#define BOOTSTRAP_MSG_ID_BASE		0x42535400u	/* 'B' 'S' 'T' 0 */
#define BOOTSTRAP_MSG_REPLY_OFFSET	0x100u

#define BOOTSTRAP_MSG_CHECK_IN		(BOOTSTRAP_MSG_ID_BASE + 1u)
#define BOOTSTRAP_MSG_LOOK_UP		(BOOTSTRAP_MSG_ID_BASE + 2u)

#define BOOTSTRAP_MSG_CHECK_IN_REPLY \
	(BOOTSTRAP_MSG_CHECK_IN + BOOTSTRAP_MSG_REPLY_OFFSET)
#define BOOTSTRAP_MSG_LOOK_UP_REPLY \
	(BOOTSTRAP_MSG_LOOK_UP + BOOTSTRAP_MSG_REPLY_OFFSET)

#define BOOTSTRAP_MAX_NAME_LEN		128

/*
 * Wire layout. mach_msg_header_t is 24 bytes; descriptor types pack
 * with #pragma pack(4) to match kernel-side mach_msg_header_t
 * alignment expectations.
 *
 * Reply is a COMPLEX message (msgh_bits has MACH_MSGH_BITS_COMPLEX
 * set) carrying the service port as a port descriptor — that's how
 * Mach delivers port rights across IPC spaces. The descriptor's
 * .name field is in the SENDER's view on the wire; the kernel
 * translates it to a name valid in the RECEIVER's IPC space on
 * delivery. For same-task (Phase G1 carry-over) the names happen
 * to match; cross-task they don't, and the receiver reads
 * descriptor.name to learn the local name.
 */
#pragma pack(4)

struct bootstrap_request_msg {
	mach_msg_header_t	header;
	char			service_name[BOOTSTRAP_MAX_NAME_LEN];
};

struct bootstrap_reply_msg {
	mach_msg_header_t		header;
	mach_msg_body_t			body;		/* descriptor_count = 1 */
	mach_msg_port_descriptor_t	port;		/* service port */
	kern_return_t			result;
};

#pragma pack()

#endif /* !_FREEBSD_LAUNCHD_MACH_BOOTSTRAP_PROTOCOL_H_ */

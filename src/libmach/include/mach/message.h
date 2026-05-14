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
#include <mach/std_types.h>	/* natural_t, integer_t, boolean_t, kern_return_t */
				/* On Apple, <mach/message.h> pulls the
				 * machine word types in transitively; we do
				 * the same so Apple-source consumers (migcom,
				 * MIG stubs, launchd-842) that include only
				 * <mach/message.h> still see natural_t etc.
				 * std_types.h's boolean_t / kern_return_t are
				 * guarded, so the typedefs below this point
				 * become no-ops — left in place for clarity. */

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

/*
 * boolean_t — Apple uses it pervasively in Mach APIs. On macOS it
 * lives in <mach/boolean.h>; we inline the typedef here so any
 * Apple-source consumer that #includes <mach/message.h> picks it
 * up. Matches Apple's `typedef unsigned int boolean_t`.
 */
#ifndef _BOOLEAN_T_DEFINED
#define _BOOLEAN_T_DEFINED
typedef unsigned int boolean_t;
#endif

/*
 * audit_token_t — opaque BSM credential token used by Mach trailers
 * and by libxpc's xpc_connection_set_credentials() /
 * xpc_dictionary_get_audit_token(). Apple defines it in <bsm/audit.h>
 * via <mach/message.h>; we inline the typedef here for the same
 * reason. Internal representation must not be inspected directly;
 * audit_token_to_au32() is the canonical accessor.
 */
#ifndef _AUDIT_TOKEN_T_DEFINED
#define _AUDIT_TOKEN_T_DEFINED
typedef struct {
	unsigned int val[8];
} audit_token_t;
#endif

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
 * MACH_MSG_TYPE_* — port-right disposition values used in msgh_bits and
 * in port-right-transfer calls (mach_port_insert_right, MIG descriptors,
 * etc.). Apple keeps the complete set in <mach/message.h>; we follow
 * suit so Apple-source consumers (migcom, MIG-generated stubs,
 * launchd-842) that #include only <mach/message.h> get all of them.
 * <mach/mach_port.h> #includes us and relies on these definitions
 * rather than re-declaring its own copy.
 *
 * The MOVE/COPY/MAKE family (16..22) are the concrete dispositions;
 * the PORT_* names are the abstract aliases MIG uses in .defs files;
 * POLYMORPHIC is the "disposition determined at runtime" sentinel.
 */
#define MACH_MSG_TYPE_PORT_NONE		0

#define MACH_MSG_TYPE_MOVE_RECEIVE	16
#define MACH_MSG_TYPE_MOVE_SEND		17
#define MACH_MSG_TYPE_MOVE_SEND_ONCE	18
#define MACH_MSG_TYPE_COPY_SEND		19
#define MACH_MSG_TYPE_MAKE_SEND		20
#define MACH_MSG_TYPE_MAKE_SEND_ONCE	21
#define MACH_MSG_TYPE_COPY_RECEIVE	22

#define MACH_MSG_TYPE_PORT_NAME		15
#define MACH_MSG_TYPE_PORT_RECEIVE	MACH_MSG_TYPE_MOVE_RECEIVE
#define MACH_MSG_TYPE_PORT_SEND		MACH_MSG_TYPE_MOVE_SEND
#define MACH_MSG_TYPE_PORT_SEND_ONCE	MACH_MSG_TYPE_MOVE_SEND_ONCE
#define MACH_MSG_TYPE_LAST		22

#define MACH_MSG_TYPE_POLYMORPHIC	((mach_msg_type_name_t)-1)

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

/*
 * Mach trailers. Receivers can request the kernel append trailer
 * elements to the received message via MACH_RCV_TRAILER_TYPE() and
 * MACH_RCV_TRAILER_ELEMENTS() in the option bitmask. The trailer
 * format/element type tuple selects which struct is appended; libxpc
 * uses MACH_RCV_TRAILER_AUDIT to pull credentials off Mach IPC.
 *
 * NB: our kernel does not currently materialize audit trailers — the
 * trailer struct fields will be zero — but Apple-source code that
 * reads them still has to type-check at compile time.
 */
typedef unsigned int mach_msg_trailer_type_t;
typedef unsigned int mach_msg_trailer_size_t;
typedef unsigned int mach_port_seqno_t;
typedef uint64_t     mach_port_context_t;

#define MACH_MSG_TRAILER_FORMAT_0	0

#define MACH_RCV_TRAILER_NULL		0
#define MACH_RCV_TRAILER_SEQNO		1
#define MACH_RCV_TRAILER_SENDER		2
#define MACH_RCV_TRAILER_AUDIT		3
#define MACH_RCV_TRAILER_CTX		4

#define MACH_RCV_TRAILER_TYPE(x)	(((x) & 0xf) << 28)
#define MACH_RCV_TRAILER_ELEMENTS(x)	(((x) & 0xf) << 24)

typedef struct {
	mach_msg_trailer_type_t		msgh_trailer_type;
	mach_msg_trailer_size_t		msgh_trailer_size;
} mach_msg_trailer_t;

typedef struct {
	unsigned int			val[2];
} security_token_t;

typedef struct {
	mach_msg_trailer_type_t		msgh_trailer_type;
	mach_msg_trailer_size_t		msgh_trailer_size;
	mach_port_seqno_t		msgh_seqno;
	security_token_t		msgh_sender;
	audit_token_t			msgh_audit;
} mach_msg_audit_trailer_t;

/* Timeout sentinel for mach_msg(): wait forever. */
#define MACH_MSG_TIMEOUT_NONE		((mach_msg_timeout_t)0)

/*
 * Boolean spellings — Apple Mach code uses TRUE/FALSE liberally; we
 * provide them only if not already defined.
 */
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

mach_msg_return_t mach_msg(
    mach_msg_header_t *msg,
    mach_msg_option_t  option,
    mach_msg_size_t    send_size,
    mach_msg_size_t    rcv_size,
    mach_port_name_t   rcv_name,
    mach_msg_timeout_t timeout,
    mach_port_name_t   notify);

/*
 * mach_msg_send / mach_msg_receive — Apple's thin convenience wrappers
 * around mach_msg(). Inline so we don't add new exported symbols to
 * libsystem_kernel; matches Apple's xnu/libsyscall implementations.
 */
static __inline mach_msg_return_t
mach_msg_send(mach_msg_header_t *msg)
{
	return mach_msg(msg, MACH_SEND_MSG, msg->msgh_size, 0,
	    MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}

static __inline mach_msg_return_t
mach_msg_receive(mach_msg_header_t *msg)
{
	return mach_msg(msg, MACH_RCV_MSG, 0, msg->msgh_size,
	    msg->msgh_local_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}

/*
 * kern_return_t — also lives in <mach/mach_port.h>, but message.h
 * needs it for mig_reply_error_t. <mach/mach_port.h> includes us, so
 * we can't pull in mach_port.h without a cycle. Define here with a
 * guard so mach_port.h's redefinition is suppressed when it follows.
 */
#ifndef _KERN_RETURN_T_DEFINED
#define _KERN_RETURN_T_DEFINED
typedef int kern_return_t;
#endif

/*
 * mig_reply_error_t — wire layout of a MIG-generated stub's reply on
 * the error path. libxpc inspects this in xpc_pipe_try_receive() to
 * decide whether to forward a reply or drop a "no reply" sentinel.
 */
typedef struct {
	mach_msg_header_t	Head;
	int			NDR;	/* unused on FreeBSD — MIG NDR header */
	kern_return_t		RetCode;
} mig_reply_error_t;

#define MIG_NO_REPLY		-305	/* matches Apple's mig_errors.h */

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MESSAGE_H_ */

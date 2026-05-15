/*
 * mach/ndr.h — Network Data Representation record for MIG.
 *
 * Every MIG-generated Mach message carries an NDR_record_t in its
 * body identifying the byte / integer / float representation of the
 * sender, so the receiver can byte-swap if it differs. migcom
 * references sizeof(NDR_record_t) when laying out generated message
 * structs; the MIG-generated stubs embed it in every request/reply.
 *
 * Layout matches Apple's <mach/ndr.h> exactly — it's wire-visible,
 * so the field order and widths are fixed. On a homogeneous
 * little-endian FreeBSD/amd64 deployment the swap paths never fire,
 * but the struct still has to be the right shape on the wire.
 */
#ifndef _MACH_NDR_H_
#define _MACH_NDR_H_

typedef struct {
	unsigned char	mig_vers;
	unsigned char	if_vers;
	unsigned char	reserved1;
	unsigned char	mig_encoding;
	unsigned char	int_rep;
	unsigned char	char_rep;
	unsigned char	float_rep;
	unsigned char	reserved2;
} NDR_record_t;

/* Representation constants — int_rep / char_rep / float_rep values. */
#define NDR_INT_BIG		0
#define NDR_INT_LITTLE		1
#define NDR_CHAR_ASCII		0
#define NDR_FLOAT_IEEE		0

/*
 * NDR_record — the singleton instance every MIG-generated client
 * stub embeds in outgoing message headers. Defined once in
 * libsystem_kernel (mach_traps.c), declared here for the stubs.
 */
extern NDR_record_t NDR_record;

#endif /* !_MACH_NDR_H_ */

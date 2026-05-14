/*
 * libkern/OSByteOrder.h — FreeBSD shim for Apple's byte-order header.
 *
 * launchd-842's liblaunch.c uses the OSSwapHostToLittleInt* /
 * OSSwapLittleToHostInt* family to serialize launch_data over the
 * wire in a fixed (little-endian) representation. FreeBSD's
 * <sys/endian.h> provides the exact equivalents — htole* / le*toh —
 * so this shim is a thin rename layer.
 */
#ifndef _LIBKERN_OSBYTEORDER_H_SHIM_
#define _LIBKERN_OSBYTEORDER_H_SHIM_

#include <sys/endian.h>
#include <stdint.h>

/* Host <-> little-endian, fixed width. */
#define OSSwapHostToLittleInt16(x)	htole16(x)
#define OSSwapHostToLittleInt32(x)	htole32(x)
#define OSSwapHostToLittleInt64(x)	htole64(x)
#define OSSwapLittleToHostInt16(x)	le16toh(x)
#define OSSwapLittleToHostInt32(x)	le32toh(x)
#define OSSwapLittleToHostInt64(x)	le64toh(x)

/* Host <-> big-endian, fixed width — not used by liblaunch today but
 * cheap to provide for the rest of the launchd-842 tree. */
#define OSSwapHostToBigInt16(x)		htobe16(x)
#define OSSwapHostToBigInt32(x)		htobe32(x)
#define OSSwapHostToBigInt64(x)		htobe64(x)
#define OSSwapBigToHostInt16(x)		be16toh(x)
#define OSSwapBigToHostInt32(x)		be32toh(x)
#define OSSwapBigToHostInt64(x)		be64toh(x)

/* Unconditional swap. */
#define OSSwapInt16(x)			bswap16(x)
#define OSSwapInt32(x)			bswap32(x)
#define OSSwapInt64(x)			bswap64(x)

#endif /* !_LIBKERN_OSBYTEORDER_H_SHIM_ */

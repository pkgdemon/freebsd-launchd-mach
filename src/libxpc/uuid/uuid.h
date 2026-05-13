/*
 * uuid/uuid.h — local shim for libxpc's `#include <uuid/uuid.h>` lines.
 *
 * libxpc's xpc/xpc.h and subr_nvlist.c follow the Apple / e2fsprogs-
 * libuuid convention of `<uuid/uuid.h>`. The actual API they call
 * (uuid_to_string, uuid_s_ok, uuid_t-as-struct) is FreeBSD's native
 * <uuid.h>, NOT libuuid's `unsigned char[16]` array shape. So we
 * redirect <uuid/uuid.h> to FreeBSD's <uuid.h>; no third-party
 * libuuid package needed.
 *
 * This shim is reachable because libxpc's Makefile adds -I${.CURDIR}
 * (the libxpc source dir) to CFLAGS, which makes <uuid/uuid.h>
 * resolve to src/libxpc/uuid/uuid.h before any system path.
 */
#ifndef _LIBXPC_UUID_UUID_H_SHIM_
#define _LIBXPC_UUID_UUID_H_SHIM_

#include <uuid.h>

#endif /* !_LIBXPC_UUID_UUID_H_SHIM_ */

/*
 * unistd.h — FreeBSD shim layered over the base header.
 *
 * launchd-842's core.c calls confstr(_CS_DARWIN_USER_TEMP_DIR, ...)
 * to fetch the per-user temp dir path that macOS hands out (one of
 * the /var/folders/.../T paths). FreeBSD has neither the constant
 * nor that confstr key — define the constant to a value confstr(3)
 * will reject (returns 0, errno=EINVAL); core.c already handles a
 * zero return as "no TMPDIR override, leave the env alone".
 */
#ifndef _FREEBSD_SHIM_UNISTD_H_
#define _FREEBSD_SHIM_UNISTD_H_

#include_next <unistd.h>

#ifndef _CS_DARWIN_USER_TEMP_DIR
#define _CS_DARWIN_USER_TEMP_DIR	65537	/* unrecognized — confstr returns 0 */
#endif
#ifndef _CS_DARWIN_USER_DIR
#define _CS_DARWIN_USER_DIR		65538
#endif
#ifndef _CS_DARWIN_USER_CACHE_DIR
#define _CS_DARWIN_USER_CACHE_DIR	65539
#endif

#endif /* _FREEBSD_SHIM_UNISTD_H_ */

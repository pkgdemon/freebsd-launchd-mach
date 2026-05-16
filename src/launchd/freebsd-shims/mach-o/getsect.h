/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Stub mach-o/getsect.h for launchctl. Apple's launchctl reads a
 * pre-cached XPC service descriptor blob from the binary's
 * __TEXT,__xpcd_cache section via getsectiondata() (line ~1717
 * in launchctl.c). Mach-O segment/section lookup is meaningless on
 * FreeBSD ELF binaries -- we ship no XPC daemon caches in __TEXT.
 *
 * Stub returns NULL so the caller's NULL-check fires and the
 * function falls through to the live-disk LaunchAgents/LaunchDaemons
 * scan (the normal path on a fresh boot anyway).
 */

#ifndef _FREEBSD_LAUNCHD_MACH_MACH_O_GETSECT_H_
#define _FREEBSD_LAUNCHD_MACH_MACH_O_GETSECT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Apple's mach_header / mach_header_64 are forward-declared here
 * so callers can pass `image_info.dli_fbase` (a void*) cast to
 * (const struct mach_header *). The struct is opaque in our stub. */
struct mach_header;
struct mach_header_64;

/* Stub: no Mach-O sections in our ELF binaries. */
static inline uint8_t *
getsectiondata(const struct mach_header_64 *mhp __unused,
    const char *segname __unused, const char *sectname __unused,
    unsigned long *size)
{
        if (size != NULL) {
                *size = 0;
        }
        return NULL;
}

/* Compatibility alias for 32-bit mach_header callers (none in
 * launchctl.c, but defining it keeps headers from breaking if
 * other Apple sources land later). */
static inline uint8_t *
getsectiondata32(const struct mach_header *mhp __unused,
    const char *segname __unused, const char *sectname __unused,
    unsigned long *size)
{
        if (size != NULL) {
                *size = 0;
        }
        return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _FREEBSD_LAUNCHD_MACH_MACH_O_GETSECT_H_ */

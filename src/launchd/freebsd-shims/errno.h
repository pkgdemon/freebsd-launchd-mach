/*
 * errno.h — FreeBSD shim layered over the base header.
 *
 * macOS errno(3) carries a few extras FreeBSD doesn't define;
 * launchd-842 compares WEXITSTATUS against them. The mapping doesn't
 * have to be exact — define them to high values above FreeBSD's
 * errno range (1..96) so they don't shadow real codes.
 */
#ifndef _FREEBSD_SHIM_ERRNO_H_
#define _FREEBSD_SHIM_ERRNO_H_

#include_next <errno.h>

#ifndef EBADARCH
#define EBADARCH	200	/* macOS: wrong CPU arch for executable */
#endif
#ifndef ESHLIBVERS
#define ESHLIBVERS	201	/* macOS: shared-lib version mismatch */
#endif

#endif /* _FREEBSD_SHIM_ERRNO_H_ */

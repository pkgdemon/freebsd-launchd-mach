/*
 * libinfo.h — FreeBSD shim for Apple's <libinfo.h>.
 *
 * macOS's Libinfo is the userland name-resolution layer (the "si"
 * module system behind getpwnam/getgrnam/getaddrinfo). launchd-842's
 * core.c reaches into it for exactly one thing: when PID 1 looks up a
 * user/group it disables the "ds" (opendirectoryd) search module and
 * the L1 cache via si_search_module_set_flags(), so a lookup that
 * raced daemon startup isn't served a stale negative cache entry.
 *
 * FreeBSD's name resolution is nsswitch-based with no equivalent
 * pluggable-module flag surface, so si_search_module_set_flags() is
 * stubbed to a no-op — getpwnam()/getgrnam() on FreeBSD don't carry
 * the negative-cache hazard this guards against.
 */
#ifndef _FREEBSD_SHIM_LIBINFO_H_
#define _FREEBSD_SHIM_LIBINFO_H_

#include <stdint.h>

static __inline void
si_search_module_set_flags(const char *name, uint32_t flags)
{
	(void)name;
	(void)flags;
}

#endif /* _FREEBSD_SHIM_LIBINFO_H_ */

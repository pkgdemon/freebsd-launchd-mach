/*
 * malloc/malloc.h — FreeBSD shim for Apple's malloc-introspection header.
 *
 * Apple's <malloc/malloc.h> exposes the zone allocator and a few
 * introspection calls. launchd-842 (core.c) uses exactly one symbol
 * from it: malloc_size(ptr), which returns the usable size of an
 * allocation. FreeBSD's jemalloc provides the same thing as
 * malloc_usable_size() in <malloc_np.h>, so this shim is a one-call
 * rename layer.
 *
 * (launchd_malloc_log_stacks, despite the name, is launchd's own
 * runtime.c global — not from this header.)
 */
#ifndef _MALLOC_MALLOC_H_SHIM_
#define _MALLOC_MALLOC_H_SHIM_

#include <stdlib.h>
#include <malloc_np.h>		/* FreeBSD: malloc_usable_size */

static __inline size_t
malloc_size(const void *ptr)
{
	return malloc_usable_size(__DECONST(void *, ptr));
}

/* malloc_zone_t — opaque on FreeBSD; the zone allocator API isn't
 * provided. Declared so any incidental `malloc_zone_t *` reference
 * type-checks; calling the zone functions would be a link error,
 * which is the right failure mode (nothing in launchd's build path
 * actually calls them). */
typedef struct _malloc_zone_t malloc_zone_t;

#endif /* !_MALLOC_MALLOC_H_SHIM_ */

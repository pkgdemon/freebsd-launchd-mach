/* os/collections.h — FreeBSD shim. Apple's intrusive-list / queue
 * macros. Most are TAILQ-style around _TAILQ_ENTRY. Forward to FreeBSD
 * sys/queue.h equivalents. libnotify uses a small subset; expand as
 * compile errors surface. */
#ifndef _FREEBSD_SHIM_OS_COLLECTIONS_H_
#define _FREEBSD_SHIM_OS_COLLECTIONS_H_

#include <sys/queue.h>

/* Apple uses os_set_t / os_map_t for hashed containers. We don't have
 * a direct equivalent in libc; if libnotify needs these, add a tiny
 * hash-table backed by malloc here. For now leave undefined and let
 * the build error tell us what's actually used. */

#endif

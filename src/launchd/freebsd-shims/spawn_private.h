/*
 * spawn_private.h — FreeBSD shim for Apple's <spawn_private.h>. The
 * Apple posix_spawn extension surface is consolidated in
 * <System/sys/spawn.h>; this header just re-includes it so
 * launchd-842's core.c #include resolves.
 */
#ifndef _FREEBSD_SHIM_SPAWN_PRIVATE_H_
#define _FREEBSD_SHIM_SPAWN_PRIVATE_H_

#include <System/sys/spawn.h>

#endif /* _FREEBSD_SHIM_SPAWN_PRIVATE_H_ */

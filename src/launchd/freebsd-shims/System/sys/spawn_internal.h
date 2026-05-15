/*
 * System/sys/spawn_internal.h — FreeBSD shim for Apple's
 * <System/sys/spawn_internal.h>. The Apple posix_spawn extension
 * surface is consolidated in <System/sys/spawn.h>; this header just
 * re-includes it so launchd-842's core.c #include resolves.
 */
#ifndef _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_INTERNAL_H_
#define _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_INTERNAL_H_

#include <System/sys/spawn.h>

#endif /* _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_INTERNAL_H_ */

/*
 * sys/reboot.h — FreeBSD shim layered over the base header.
 *
 * runtime.c's debug-printer FLAGIF() decodes reboot(2) flag bits
 * into human-readable names. macOS defines four flags FreeBSD does
 * not — RB_ALTBOOT / RB_UNIPROC / RB_SAFEBOOT / RB_UPSDELAY. Define
 * them as zero: the if (flags & X) probe never matches, the name
 * simply doesn't appear in the log line, and the corresponding bits
 * stay free on FreeBSD where they'd otherwise collide with RB_KDB /
 * RB_RDONLY / RB_DUMP / RB_MINIROOT.
 */
#ifndef _FREEBSD_SHIM_SYS_REBOOT_H_
#define _FREEBSD_SHIM_SYS_REBOOT_H_

#include_next <sys/reboot.h>

#ifndef RB_ALTBOOT
#define RB_ALTBOOT	0
#endif
#ifndef RB_UNIPROC
#define RB_UNIPROC	0
#endif
#ifndef RB_SAFEBOOT
#define RB_SAFEBOOT	0
#endif
#ifndef RB_UPSDELAY
#define RB_UPSDELAY	0
#endif

#endif /* _FREEBSD_SHIM_SYS_REBOOT_H_ */

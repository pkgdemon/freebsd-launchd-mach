/*
 * mach/host_reboot.h — host_reboot() flags. launchd-842's core.c
 * calls host_reboot(mach_host_self(), HOST_REBOOT_DEBUGGER) on the
 * crash-into-debugger paths. On freebsd-launchd-mach host_reboot() in
 * libsystem_kernel maps onto FreeBSD's reboot(2) (HOST_REBOOT_HALT ->
 * RB_HALT, HOST_REBOOT_DEBUGGER -> RB_DEBUG, ...).
 */
#ifndef _MACH_HOST_REBOOT_H_
#define _MACH_HOST_REBOOT_H_

#include <mach/mach_types.h>	/* host_priv_t, kern_return_t */

#define HOST_REBOOT_HALT	0x1
#define HOST_REBOOT_UPSPIN	0x2	/* deprecated on Apple too */
#define HOST_REBOOT_DEBUGGER	0x4

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t host_reboot(host_priv_t host_priv, int options);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_HOST_REBOOT_H_ */

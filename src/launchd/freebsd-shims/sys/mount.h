/*
 * sys/mount.h — FreeBSD shim layered over the base header.
 *
 * launchd-842's core.c watches volume-quota-state changes via the
 * macOS VQ_UPDATE notify flag. FreeBSD has its own VQ_* set
 * (VQ_NOTRESP / NEEDAUTH / LOWDISK / MOUNT / UNMOUNT / DEAD / ASSIST
 * / NOTRESPLOCK / QUOTA / NEARLOWDISK / DESIRED_DISK) but no
 * VQ_UPDATE. The shim pulls in the base header and adds an unused
 * bit; the corresponding code path runs but the notify it watches
 * for never fires on FreeBSD.
 */
#ifndef _FREEBSD_SHIM_SYS_MOUNT_H_
#define _FREEBSD_SHIM_SYS_MOUNT_H_

#include_next <sys/mount.h>

#ifndef VQ_UPDATE
#define VQ_UPDATE	0x10000	/* placeholder, above the FreeBSD VQ_* set */
#endif

#endif /* _FREEBSD_SHIM_SYS_MOUNT_H_ */

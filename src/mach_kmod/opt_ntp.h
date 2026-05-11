/*
 * opt_ntp.h — matches FreeBSD GENERIC kernel config.
 *
 * GENERIC does NOT set `options PPS_SYNC`. Mach's clock code includes
 * this header to gate PPS_SYNC-conditional code paths off. Empty is
 * the correct content for a GENERIC-targeted module build.
 *
 * If your kernel sets PPS_SYNC, rebuild with:
 *
 *     make KERNBUILDDIR=/usr/obj/usr/src/sys/MYKERNEL
 *
 * bsd.kmod.mk will then use your kernel's opt_ntp.h instead.
 */

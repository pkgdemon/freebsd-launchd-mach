/*
 * opt_capsicum.h — matches FreeBSD GENERIC kernel config.
 *
 * GENERIC enables Capsicum via `options CAPABILITY_MODE` and
 * `options CAPABILITIES`. The latter is the symbol kernel code (and
 * some Mach IPC paths around cred handling) tests via #ifdef.
 *
 * If your kernel was built with a config that omits CAPABILITIES,
 * rebuild this module with:
 *
 *     make KERNBUILDDIR=/usr/obj/usr/src/sys/MYKERNEL
 *
 * bsd.kmod.mk reads opt_capsicum.h from KERNBUILDDIR/ then, overriding
 * this file.
 */

#define CAPABILITIES 1

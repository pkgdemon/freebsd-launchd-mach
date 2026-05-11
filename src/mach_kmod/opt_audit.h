/*
 * opt_audit.h — matches FreeBSD GENERIC kernel config.
 *
 * GENERIC sets `options AUDIT` to enable the Security event auditing
 * subsystem. <security/audit/audit.h> gates audit_sysclose() (and the
 * rest of the audit API) behind #ifdef AUDIT — without this define
 * the prototypes are invisible and ipc_entry.c's direct
 * audit_sysclose() calls fail with -Werror,-Wimplicit-function-
 * declaration.
 *
 * Matches running kernels built with GENERIC. If a custom kernel
 * disables AUDIT, rebuild this module with:
 *
 *     make KERNBUILDDIR=/usr/obj/usr/src/sys/MYKERNEL
 *
 * bsd.kmod.mk will then use that kernel's opt_audit.h instead.
 */

#define AUDIT 1

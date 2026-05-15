/*
 * bsm/auditd_lib.h — FreeBSD shim for macOS <bsm/auditd_lib.h>.
 *
 * Apple's auditd_lib is the launchd<->auditd control surface:
 * audit_quick_start() / audit_quick_stop() bring the audit daemon up
 * at boot and down at shutdown. On FreeBSD the audit daemon is
 * managed by rc(8), not launchd, and OpenBSM in base ships no
 * auditd_lib. launchd-842 only calls these from PID 1 paths — which
 * this Phase I1c build does not exercise — so stub them to success.
 * A real port would hand audit lifecycle to FreeBSD's auditd rc
 * script rather than driving it from launchd.
 */
#ifndef _FREEBSD_SHIM_BSM_AUDITD_LIB_H_
#define _FREEBSD_SHIM_BSM_AUDITD_LIB_H_

static __inline int audit_quick_start(void) { return 0; }
static __inline int audit_quick_stop(void) { return 0; }

#endif /* _FREEBSD_SHIM_BSM_AUDITD_LIB_H_ */

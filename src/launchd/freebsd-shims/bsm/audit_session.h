/*
 * bsm/audit_session.h — FreeBSD shim for macOS <bsm/audit_session.h>.
 *
 * Apple's audit-session API ties Mach tasks to audit session IDs and
 * hands out a per-session Mach port (audit_session_self,
 * audit_session_port, audit_session_join). FreeBSD's OpenBSM has the
 * audit-session *identifiers* (au_asid_t, getaudit_addr/setaudit_addr
 * live in base <bsm/audit.h> + libbsm) but not the Mach-port half of
 * the API — that's a macOS-kernel construct.
 *
 * launchd uses the session ports for per-job audit isolation. Until
 * that's ported, stub the macOS-only entry points so callers take
 * their already-present degraded paths: core.c checks the returned
 * port against MACH_PORT_NULL and treats errno == ENOSYS from
 * audit_session_port() as "no per-session port, carry on".
 *
 * NB: FreeBSD's base <bsm/audit.h> already *declares* (non-static)
 * audit_session_self() and audit_session_join(), plus AU_ASSIGN_ASID
 * / AU_DEFAUDITSID — so this shim must not redefine those. It only
 * supplies the entry points FreeBSD's audit.h omits:
 * _audit_session_self(), audit_session_port(), AU_SESSION_FLAG_*.
 */
#ifndef _FREEBSD_SHIM_BSM_AUDIT_SESSION_H_
#define _FREEBSD_SHIM_BSM_AUDIT_SESSION_H_

#include <errno.h>
#include <bsm/audit.h>
#include <mach/mach.h>

#ifndef AU_SESSION_FLAG_IS_INITIAL
#define AU_SESSION_FLAG_IS_INITIAL	0x0001
#endif

static __inline mach_port_t
_audit_session_self(void)
{
	return MACH_PORT_NULL;
}

static __inline int
audit_session_port(au_asid_t asid __unused, mach_port_t *port)
{
	if (port != NULL) {
		*port = MACH_PORT_NULL;
	}
	errno = ENOSYS;
	return -1;
}

#endif /* _FREEBSD_SHIM_BSM_AUDIT_SESSION_H_ */

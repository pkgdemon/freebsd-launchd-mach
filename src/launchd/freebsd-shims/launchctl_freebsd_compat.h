/*
 * launchctl_freebsd_compat.h — freebsd-launchd-mach (2026-05-15)
 *
 * Compatibility defines for launchctl.c. Force-included via the
 * Makefile (-include) so launchctl.c sees these identifiers without
 * patching its source.
 *
 * Each define is paired with a brief justification — none of these
 * code paths are reached by `launchctl version` (the LAUNCHCTL-BUILD-OK
 * smoke target). The shimmed values let the affected functions
 * compile; at runtime they either return safely-failing values
 * (sysctl ENOENT) or touch a harmless file path.
 */
#ifndef _FREEBSD_LAUNCHD_MACH_LAUNCHCTL_COMPAT_H_
#define _FREEBSD_LAUNCHD_MACH_LAUNCHCTL_COMPAT_H_

/* sys/wait.h
 *   launchctl.c uses WIFEXITED, WIFSIGNALED, WEXITSTATUS, WTERMSIG,
 *   waitpid in 8+ sites without including <sys/wait.h>. On macOS the
 *   header gets pulled in transitively via <sys/types.h> or similar;
 *   FreeBSD requires explicit inclusion. Adding it here keeps
 *   launchctl.c source unchanged.
 */
#include <sys/wait.h>

/* mach/task.h task_for_pid()
 *   launchctl.c:3591 uses task_for_pid() to translate a pid into the
 *   target task port for a Mach trap on the named process. Apple's
 *   Mach implements this; our libmach doesn't (and the underlying
 *   trap requires task-for-pid entitlements on macOS even there).
 *   Stub returns KERN_FAILURE so the caller's KERN_SUCCESS check
 *   fails and the function bails. Inline static so each TU that
 *   includes this header gets a private definition; no link conflict.
 */
#include <stddef.h>		/* NULL */
#include <mach/mach_types.h>
#include <mach/kern_return.h>
static inline kern_return_t
task_for_pid(mach_port_name_t target_tport __attribute__((unused)),
    int pid __attribute__((unused)),
    mach_port_name_t *t)
{
        if (t != NULL) *t = MACH_PORT_NULL;
        return KERN_FAILURE;
}

/* _PATH_UTMPX (line 2441 in launchctl.c)
 *   touch_file(_PATH_UTMPX, DEFFILEMODE) — bumps the utmpx login
 *   record at boot. FreeBSD's <paths.h> doesn't define _PATH_UTMPX
 *   (utmpx file lives at /var/log/utx.* but the path macro is private
 *   to libulog). Pointing at /var/empty makes touch_file a no-op.
 */
#ifndef _PATH_UTMPX
#define _PATH_UTMPX     "/var/empty/utmpx"
#endif

/* KERN_SAFEBOOT (line 4181) and KERN_NETBOOT (line 4195)
 *   is_safeboot() and is_netboot() do sysctl(CTL_KERN, KERN_*) reads.
 *   FreeBSD has no equivalent (we don't have a "safe boot" mode and
 *   netboot is detected differently). Use sysctl MIB indexes
 *   guaranteed to ENOENT so sysctl(2) fails cleanly and the helpers
 *   return false.
 */
#ifndef KERN_SAFEBOOT
#define KERN_SAFEBOOT   9990
#endif
#ifndef KERN_NETBOOT
#define KERN_NETBOOT    9991
#endif

/* KERN_OSVERSION (line 4364)
 *   do_sysversion_sysctl() reads kern.osversion to print the macOS
 *   build number ("24A335" etc.). FreeBSD's analogue is
 *   KERN_OSRELEASE (returns "15.0-RELEASE"); not byte-identical but
 *   same protocol (string sysctl). Alias.
 */
#ifndef KERN_OSVERSION
#define KERN_OSVERSION  KERN_OSRELEASE
#endif

/* SO_EXECPATH (line 4447)
 *   setsockopt(SOL_SOCKET, SO_EXECPATH, ...) — Apple-only socket
 *   option that tags the socket with the binary path allowed to
 *   inherit it. The launchctl call site explicitly tolerates
 *   ENOPROTOOPT (errno-check below the call). Use an unused
 *   socket-option value so the kernel returns ENOPROTOOPT.
 */
#ifndef SO_EXECPATH
#define SO_EXECPATH     0xfffe
#endif

#endif /* _FREEBSD_LAUNCHD_MACH_LAUNCHCTL_COMPAT_H_ */

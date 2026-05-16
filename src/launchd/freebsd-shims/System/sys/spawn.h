/*
 * System/sys/spawn.h — FreeBSD shim for Apple's posix_spawn
 * extensions. launchd-842's core.c drives job execution through
 * posix_spawn() with a pile of Apple-only attributes: the SETEXEC /
 * START_SUSPENDED spawn flags, process-type bands, and the
 * posix_spawnattr_*_np() setters.
 *
 * FreeBSD's base <spawn.h> has standard POSIX_SPAWN_SET* flags but
 * none of the Apple extensions. This header (the single source of
 * truth — <System/sys/spawn_internal.h> and <spawn_private.h> just
 * re-include it) supplies the missing constants and stubs the _np
 * setters to success: on FreeBSD the corresponding kernel knobs do
 * not exist, so the attributes are silently dropped and posix_spawn()
 * runs with standard POSIX semantics. The job-exec path is not
 * exercised by the Phase I1c no-IPC CLI smoke anyway.
 */
#ifndef _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_H_
#define _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>		/* snprintf (iter-14 wrapper-trace) */
#include <errno.h>
#include <unistd.h>
#include <spawn.h>		/* posix_spawnattr_t, posix_spawn (FreeBSD base) */
#include <mach/machine.h>	/* cpu_type_t */

/*
 * Apple posix_spawn flags. Values are placed above FreeBSD's base
 * POSIX_SPAWN_SET* bits (0x01..0x20) to avoid collision when OR'd
 * into the same short flag word.
 */
#ifndef POSIX_SPAWN_SETEXEC
#define POSIX_SPAWN_SETEXEC		0x0040
#endif
#ifndef POSIX_SPAWN_START_SUSPENDED
#define POSIX_SPAWN_START_SUSPENDED	0x0080
#endif
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR	0x0100
#endif
#ifndef POSIX_SPAWN_IOS_INTERACTIVE
#define POSIX_SPAWN_IOS_INTERACTIVE	0x0200
#endif
#ifndef POSIX_SPAWN_JETSAM_HIWATER_BACKGROUND
#define POSIX_SPAWN_JETSAM_HIWATER_BACKGROUND		0x0400
#endif
#ifndef POSIX_SPAWN_JETSAM_USE_EFFECTIVE_PRIORITY
#define POSIX_SPAWN_JETSAM_USE_EFFECTIVE_PRIORITY	0x0800
#endif

/*
 * Apple QoS tiers (latency + throughput). On macOS these live in
 * <sys/qos.h>; launchd-842 references the "launch default" tiers
 * from core.c for the job-spawn QoS attributes. FreeBSD has no QoS
 * tier mechanism — values are placeholder, swallowed by the no-op
 * posix_spawnattr_*_np() setters below.
 */
#ifndef LATENCY_QOS_TIER_UNSPECIFIED
#define LATENCY_QOS_TIER_UNSPECIFIED		0x0
#define LATENCY_QOS_TIER_0			((0xFF<<16) | 1)
#define LATENCY_QOS_TIER_1			((0xFF<<16) | 2)
#define LATENCY_QOS_TIER_2			((0xFF<<16) | 3)
#define LATENCY_QOS_TIER_3			((0xFF<<16) | 4)
#define LATENCY_QOS_TIER_4			((0xFF<<16) | 5)
#define LATENCY_QOS_TIER_5			((0xFF<<16) | 6)
#define LATENCY_QOS_LAUNCH_DEFAULT_TIER		LATENCY_QOS_TIER_3

#define THROUGHPUT_QOS_TIER_UNSPECIFIED		0x0
#define THROUGHPUT_QOS_TIER_0			((0xFE<<16) | 1)
#define THROUGHPUT_QOS_TIER_1			((0xFE<<16) | 2)
#define THROUGHPUT_QOS_TIER_2			((0xFE<<16) | 3)
#define THROUGHPUT_QOS_TIER_3			((0xFE<<16) | 4)
#define THROUGHPUT_QOS_TIER_4			((0xFE<<16) | 5)
#define THROUGHPUT_QOS_TIER_5			((0xFE<<16) | 6)
#define THROUGHPUT_QOS_LAUNCH_DEFAULT_TIER	THROUGHPUT_QOS_TIER_3
#endif

/* Apple process-type bands passed to posix_spawnattr_setprocesstype_np(). */
#define POSIX_SPAWN_PROC_TYPE_APP_DEFAULT		0x00000001
#define POSIX_SPAWN_PROC_TYPE_APP_TAL			0x00000002
#define POSIX_SPAWN_PROC_TYPE_DAEMON_STANDARD		0x00000100
#define POSIX_SPAWN_PROC_TYPE_DAEMON_INTERACTIVE	0x00000200
#define POSIX_SPAWN_PROC_TYPE_DAEMON_BACKGROUND		0x00000300
#define POSIX_SPAWN_PROC_TYPE_DAEMON_ADAPTIVE		0x00000400

/*
 * Apple posix_spawnattr_*_np() setters. No FreeBSD equivalent — stub
 * to success so the (best-effort, job_assumes_zero-wrapped) callers
 * see "attribute accepted" and posix_spawn() proceeds normally.
 */
static __inline int
posix_spawnattr_setprocesstype_np(posix_spawnattr_t *attr, uint64_t type)
{
	(void)attr;
	(void)type;
	return 0;
}

static __inline int
posix_spawnattr_setbinpref_np(posix_spawnattr_t *attr, size_t count,
    cpu_type_t *pref, size_t *ocount)
{
	(void)attr;
	(void)pref;
	if (ocount != NULL) {
		*ocount = count;
	}
	return 0;
}

static __inline int
posix_spawnattr_setmacpolicyinfo_np(posix_spawnattr_t *attr,
    const char *policyname, void *policyinfo, size_t policyinfosize)
{
	(void)attr;
	(void)policyname;
	(void)policyinfo;
	(void)policyinfosize;
	return 0;
}

static __inline int
posix_spawnattr_set_importancewatch_port_np(posix_spawnattr_t *attr,
    int count, mach_port_t portarray[])
{
	(void)attr;
	(void)count;
	(void)portarray;
	return 0;
}

static __inline int
posix_spawnattr_setcpumonitor_default(posix_spawnattr_t *attr)
{
	(void)attr;
	return 0;
}

/*
 * posix_spawn / posix_spawnp wrappers that emulate Apple's
 * POSIX_SPAWN_SETEXEC.
 *
 * Apple semantic of POSIX_SPAWN_SETEXEC: "do not fork; replace the
 * calling process with the new program." launchd's job_start_child
 * sets this flag unconditionally and runs posix_spawn from the
 * already-forked child of runtime_fork(), so the child becomes the
 * job's program in-place. The PID launchd watches via EVFILT_PROC
 * is the same one that ends up running getty/etc.
 *
 * FreeBSD's libc posix_spawn has no SETEXEC equivalent and its
 * POSIX_SPAWN_VALID_FLAGS is 0x3F -- our shim's SETEXEC bit 0x40
 * is outside that mask, so FreeBSD either rejects the call with
 * EINVAL or silently ignores SETEXEC and fork+execs. Either way
 * the runtime_fork child is *not* replaced by getty: with EINVAL
 * it _exit(errno)s without ever exec'ing; with silent-ignore it
 * spawns a *grandchild* getty and then _exit(0)s itself, leaving
 * launchd's EVFILT_PROC watching the dead middle process while the
 * grandchild orphan reparents to PID 1 (launchd) anonymously.
 *
 * PID-1 launchd boot symptom that drove this fix (iter 13): the
 * com.apple.getty.plist job is loaded, dispatched -> ok, the parent
 * forks (one sysctlbyname-in-child trace at runtime.c:719) -- then
 * 8 minutes of silence, no login: prompt. The exec is what's
 * missing.
 *
 * Fix: intercept posix_spawn / posix_spawnp. If SETEXEC is set in
 * the attrs, call execve / execvp directly so the current process
 * is replaced. Otherwise pass through to libc's real posix_spawn.
 *
 * The static-inline definitions are parsed before the #define
 * macros below, so `posix_spawn(...)` inside the wrapper body
 * resolves to libc's real symbol declared by <spawn.h>. Subsequent
 * callers (launchd's core.c) get the macro substitution and reach
 * the wrappers.
 *
 * Side note: launchd captures psf = posix_spawn or posix_spawnp into
 * a function pointer (core.c:4747). After macro substitution this
 * becomes psf = freebsd_shim_posix_spawn -- still a valid function
 * symbol, no breakage.
 */
static __inline int
freebsd_shim_posix_spawn(pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *sa,
    char *const argv[], char *const envp[])
{
	short flags = 0;
	if (sa != NULL) {
		(void)posix_spawnattr_getflags(sa, &flags);
	}
	/* iter 14 diagnostic: prove the wrapper is reachable from the
	 * launchd child. stderr is wired to /dev/console at PID-1 boot
	 * via init.sh, and the runtime_fork child inherits that fd. */
	{
		char dbg[256];
		int n = snprintf(dbg, sizeof(dbg),
		    "freebsd_shim_posix_spawn: path=%s flags=0x%x SETEXEC=%d\n",
		    path ? path : "(null)", (unsigned)flags,
		    (flags & POSIX_SPAWN_SETEXEC) ? 1 : 0);
		if (n > 0) {
			ssize_t w = write(2, dbg, (size_t)n);
			(void)w;
		}
	}
	if (flags & POSIX_SPAWN_SETEXEC) {
		(void)execve(path, argv, envp);
		return errno;	/* execve only returns on failure */
	}
	return posix_spawn(pid, path, fa, sa, argv, envp);
}

static __inline int
freebsd_shim_posix_spawnp(pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *sa,
    char *const argv[], char *const envp[])
{
	short flags = 0;
	if (sa != NULL) {
		(void)posix_spawnattr_getflags(sa, &flags);
	}
	if (flags & POSIX_SPAWN_SETEXEC) {
		/*
		 * FreeBSD has no execvpe (GNU extension). launchd always
		 * passes the process's environ as envp at the spawn site
		 * (core.c:4753), and the child inherits environ from the
		 * parent across runtime_fork, so execvp -- which uses the
		 * child's environ implicitly -- produces the same result.
		 */
		(void)envp;
		(void)execvp(file, argv);
		return errno;
	}
	return posix_spawnp(pid, file, fa, sa, argv, envp);
}

#define posix_spawn	freebsd_shim_posix_spawn
#define posix_spawnp	freebsd_shim_posix_spawnp

#endif /* _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_H_ */

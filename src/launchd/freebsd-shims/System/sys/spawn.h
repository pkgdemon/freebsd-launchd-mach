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
#include <spawn.h>		/* posix_spawnattr_t (FreeBSD base) */
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

#endif /* _FREEBSD_SHIM_SYSTEM_SYS_SPAWN_H_ */

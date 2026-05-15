/*
 * sys/kern_memorystatus.h — FreeBSD shim for macOS
 * <sys/kern_memorystatus.h>.
 *
 * macOS's kern_memorystatus is the jetsam subsystem: a priority-band
 * scheme the kernel uses to pick processes to kill under memory
 * pressure. FreeBSD has no jetsam.
 *
 * launchd-842's core.c uses the JETSAM_PRIORITY_* band constants
 * unconditionally (to build its band-priority map table) but only
 * calls memorystatus_control() inside #if TARGET_OS_EMBEDDED, which
 * is 0 on this port. So in practice this shim only needs the band
 * constants; the control entry point + types are supplied anyway for
 * completeness, stubbed to fail with ENOSYS.
 *
 * Band values follow Apple's canonical ordering.
 */
#ifndef _FREEBSD_SHIM_SYS_KERN_MEMORYSTATUS_H_
#define _FREEBSD_SHIM_SYS_KERN_MEMORYSTATUS_H_

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>

/* Jetsam priority bands. */
#define JETSAM_PRIORITY_REVISION		2
#define JETSAM_PRIORITY_IDLE			0
#define JETSAM_PRIORITY_IDLE_DEFERRED		1
#define JETSAM_PRIORITY_BACKGROUND_OPPORTUNISTIC 2
#define JETSAM_PRIORITY_BACKGROUND		3
#define JETSAM_PRIORITY_MAIL			4
#define JETSAM_PRIORITY_PHONE			5
#define JETSAM_PRIORITY_UI_SUPPORT		8
#define JETSAM_PRIORITY_FOREGROUND_SUPPORT	9
#define JETSAM_PRIORITY_FOREGROUND		10
#define JETSAM_PRIORITY_AUDIO_AND_ACCESSORY	12
#define JETSAM_PRIORITY_CONDUCTOR		13
#define JETSAM_PRIORITY_HOME			16
#define JETSAM_PRIORITY_EXECUTIVE		17
#define JETSAM_PRIORITY_IMPORTANT		18
#define JETSAM_PRIORITY_CRITICAL		19
#define JETSAM_PRIORITY_TELEPHONY		20
#define JETSAM_PRIORITY_MAX			21
#define JETSAM_PRIORITY_RESERVED		21
#define JETSAM_PRIORITY_DEFAULT			18

/* memorystatus_control() commands. */
#define MEMORYSTATUS_CMD_GET_PRIORITY_LIST		1
#define MEMORYSTATUS_CMD_SET_PRIORITY_PROPERTIES	2
#define MEMORYSTATUS_CMD_GET_JETSAM_SNAPSHOT		3
#define MEMORYSTATUS_CMD_GET_PRESSURE_STATUS		4
#define MEMORYSTATUS_CMD_SET_JETSAM_HIGH_WATER_MARK	5

typedef struct memorystatus_priority_properties {
	int32_t		priority;
	uint64_t	user_data;
} memorystatus_priority_properties_t;

static __inline int
memorystatus_control(uint32_t command, int32_t pid, uint32_t flags,
    void *buffer, size_t buffersize)
{
	(void)command;
	(void)pid;
	(void)flags;
	(void)buffer;
	(void)buffersize;
	errno = ENOSYS;
	return -1;
}

#endif /* _FREEBSD_SHIM_SYS_KERN_MEMORYSTATUS_H_ */

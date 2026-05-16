/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Stub IOKit shim for launchctl. Apple's launchctl uses IOKit
 * for two things, both unrelated to launchctl's core function
 * (load/unload/list/start/stop/etc.):
 *
 *   1. IOKitWaitQuiet — waits for kexts to settle before reboot.
 *      We don't have kexts. Stub returns kIOReturnSuccess.
 *
 *   2. IORegistryEntry{FromPath,CreateCFProperty} + IOObjectRelease
 *      — looks up "IODeviceTree:/chosen" to read kBootRootActiveKey
 *      (does the host boot from a network image?). We always boot
 *      from disk. Stub returns IO_OBJECT_NULL so callers fall through.
 *
 * Apple's launchd-842 IOKit usage is restricted to these four APIs.
 * If launchctl ever picks up more IOKit calls, extend this header.
 */

#ifndef _FREEBSD_LAUNCHD_MACH_IOKIT_IOKITLIB_H_
#define _FREEBSD_LAUNCHD_MACH_IOKIT_IOKITLIB_H_

#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_port.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque IOKit handle types. macOS aliases these to mach_port_t. */
typedef mach_port_t io_object_t;
typedef io_object_t io_service_t;
typedef io_object_t io_registry_entry_t;

/* Sentinels. */
#define IO_OBJECT_NULL          ((io_object_t)0)
#define kIOMasterPortDefault    ((mach_port_t)0)

/* IOReturn-style status. macOS uses int32_t. */
typedef int32_t IOReturn;
#define kIOReturnSuccess        0

/* Wait-time spec used by IOKitWaitQuiet. macOS struct mach_timespec_t.
 * launchctl only ever passes a pointer to a stack value; the kext-quiet
 * stub doesn't consume it, so an opaque struct is fine. Use plain int
 * to avoid pulling in <mach/clock_types.h> (clock_res_t lives there). */
typedef struct {
        unsigned int    tv_sec;
        int             tv_nsec;
} freebsd_mach_timespec_t;
#ifndef mach_timespec_t
#define mach_timespec_t freebsd_mach_timespec_t
#endif

/* Stub: kexts don't exist on FreeBSD; nothing to wait for. */
static inline IOReturn
IOKitWaitQuiet(mach_port_t mp __unused, mach_timespec_t *wt __unused)
{
        return kIOReturnSuccess;
}

/* Stub: no IODeviceTree on FreeBSD; return null so callers exit early. */
static inline io_registry_entry_t
IORegistryEntryFromPath(mach_port_t mp __unused, const char *path __unused)
{
        return IO_OBJECT_NULL;
}

/* Stub: never called — we always return IO_OBJECT_NULL above, so the
 * caller's NULL-check fires before this is reached. Defined for the
 * compiler. */
static inline CFTypeRef
IORegistryEntryCreateCFProperty(io_registry_entry_t entry __unused,
    CFStringRef key __unused, CFAllocatorRef allocator __unused,
    uint32_t options __unused)
{
        return NULL;
}

/* Stub: no-op release for the null handle. */
static inline IOReturn
IOObjectRelease(io_object_t obj __unused)
{
        return kIOReturnSuccess;
}

#ifdef __cplusplus
}
#endif

#endif /* _FREEBSD_LAUNCHD_MACH_IOKIT_IOKITLIB_H_ */

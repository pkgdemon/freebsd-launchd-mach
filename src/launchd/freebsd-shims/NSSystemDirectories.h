/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Stub NSSystemDirectories for launchctl. Apple's launchctl uses
 * NSStartSearchPathEnumeration / NSGetNextSearchPathEnumeration to
 * walk the Library directory hierarchy (~/Library, /Library,
 * /Network/Library, /System/Library) looking for LaunchAgents and
 * LaunchDaemons subdirs.
 *
 * Stub returns ONLY /Local/Library and /System/Library (per the
 * project install-layout spike: /Local/Library replaces Apple's
 * /Library on this platform). Skips ~/Library (no per-user agent
 * support yet) and /Network/Library (no NetInfo / OD).
 *
 * The enumeration state is just an int counter:
 *   0 -> /Local/Library, advance to 1
 *   1 -> /System/Library, advance to 2
 *   2 -> end (return 0)
 *
 * If a future task adds per-user agents, extend the table at the
 * bottom of this header.
 */

#ifndef _FREEBSD_LAUNCHD_MACH_NSSYSTEMDIRECTORIES_H_
#define _FREEBSD_LAUNCHD_MACH_NSSYSTEMDIRECTORIES_H_

#include <string.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Apple's enum values. We only honor NSLibraryDirectory; the others
 * exist so the enum compiles, but launchctl doesn't pass them. */
typedef enum {
        NSApplicationDirectory          = 1,
        NSDemoApplicationDirectory      = 2,
        NSDeveloperApplicationDirectory = 3,
        NSAdminApplicationDirectory     = 4,
        NSLibraryDirectory              = 5,
        NSDeveloperDirectory            = 6,
        NSUserDirectory                 = 7,
        NSDocumentationDirectory        = 8,
        NSDocumentDirectory             = 9,
        NSAllApplicationsDirectory      = 100,
        NSAllLibrariesDirectory         = 101,
} NSSearchPathDirectory;

typedef enum {
        NSUserDomainMask        = 1,
        NSLocalDomainMask       = 2,
        NSNetworkDomainMask     = 4,
        NSSystemDomainMask      = 8,
        NSAllDomainsMask        = 0x0ffff,
} NSSearchPathDomainMask;

/* Apple makes this opaque; we use it as a 1-based step counter. */
typedef unsigned int NSSearchPathEnumerationState;

/* Initialize enumeration; return first valid state (1) or 0 if no
 * paths to enumerate. We always return 1 (we have at least
 * /Local/Library to walk). */
static inline NSSearchPathEnumerationState
NSStartSearchPathEnumeration(NSSearchPathDirectory dir __unused,
    NSSearchPathDomainMask mask __unused)
{
        return 1;
}

/* Advance enumeration. On entry, state is the step we're producing.
 * Fill 'path' with the corresponding directory; return next step
 * number, or 0 to terminate.
 *
 * Only NSLibraryDirectory is honored. Other directories yield empty
 * enumeration (terminator on first call). */
static inline NSSearchPathEnumerationState
NSGetNextSearchPathEnumeration(NSSearchPathEnumerationState state,
    char path[/* PATH_MAX */])
{
        switch (state) {
        case 1:
                strlcpy(path, "/Local/Library", MAXPATHLEN);
                return 2;
        case 2:
                strlcpy(path, "/System/Library", MAXPATHLEN);
                return 0;
        default:
                return 0;
        }
}

#ifdef __cplusplus
}
#endif

#endif /* _FREEBSD_LAUNCHD_MACH_NSSYSTEMDIRECTORIES_H_ */

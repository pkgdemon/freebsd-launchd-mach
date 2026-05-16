/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Stub for Apple's <bootfiles.h> (bootcaches / BootRoot constants).
 * launchctl.c references exactly one symbol: kBootRootActiveKey,
 * passed to IORegistryEntryCreateCFProperty in the
 * is_netboot-style check (line ~4511). That call site is dead
 * code on FreeBSD -- our IOKit shim's IORegistryEntryFromPath
 * returns IO_OBJECT_NULL, the os_assumes(chosen) check fails, and
 * the function returns before kBootRootActiveKey is reached.
 *
 * Provide the constant as a string literal so the compile parses.
 * Value mirrors Apple's bootfiles.h definition for reference.
 */
#ifndef _FREEBSD_LAUNCHD_MACH_BOOTFILES_H_
#define _FREEBSD_LAUNCHD_MACH_BOOTFILES_H_

#define kBootRootActiveKey "boot-root-active"

#endif

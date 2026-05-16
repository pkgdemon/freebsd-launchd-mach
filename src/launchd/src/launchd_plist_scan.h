/*
 * launchd_plist_scan.h — freebsd-launchd-mach (2026-05-16)
 *
 * In-launchd LaunchDaemons plist scanner.
 * Implementation in launchd_plist_scan.c.
 */
#ifndef _LAUNCHD_PLIST_SCAN_H_
#define _LAUNCHD_PLIST_SCAN_H_

/*
 * Walk /System/Library/LaunchDaemons + /Library/LaunchDaemons and
 * call job_import() on every .plist found. Logs summary to syslog +
 * console. Idempotent w.r.t. already-loaded jobs (job_import handles
 * duplicates). Called from launchd.c's PID-1 boot path.
 */
void launchd_scan_launchdaemons(void);

#endif /* _LAUNCHD_PLIST_SCAN_H_ */

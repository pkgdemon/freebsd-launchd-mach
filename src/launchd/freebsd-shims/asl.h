/*
 * asl.h — FreeBSD shim for Apple's <asl.h> (Apple System Log).
 *
 * launchd-842's core.c carries a vestigial `#include <asl.h>` but
 * references no ASL symbol — its logging goes through launchd's own
 * launchd_syslog() (log.c), which is backed by syslog(3) on this
 * port. So this shim is intentionally empty; if a future code path
 * needs the ASL client API it should be ported to syslog or replaced
 * outright rather than emulated here.
 */
#ifndef _FREEBSD_SHIM_ASL_H_
#define _FREEBSD_SHIM_ASL_H_

#endif /* _FREEBSD_SHIM_ASL_H_ */

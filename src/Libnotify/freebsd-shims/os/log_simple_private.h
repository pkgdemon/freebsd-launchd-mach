/* os/log_simple_private.h — FreeBSD shim. Apple-private simple-log API. */
#ifndef _FREEBSD_SHIM_OS_LOG_SIMPLE_PRIVATE_H_
#define _FREEBSD_SHIM_OS_LOG_SIMPLE_PRIVATE_H_

#include <stdio.h>
#include <stdarg.h>

/* Apple's os_log_simple* prototypes — stubbed to vfprintf(stderr). */
#define os_log_simple(level, msg)		(void)fprintf(stderr, "%s\n", (msg))
#define os_log_simple_with_pid(level, pid, msg)	(void)fprintf(stderr, "[%d] %s\n", (int)(pid), (msg))

#endif

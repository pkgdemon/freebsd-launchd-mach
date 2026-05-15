/*
 * util.h — FreeBSD shim for Apple's <util.h>.
 *
 * macOS's <util.h> exposes the pty / login helpers (openpty, forkpty,
 * login_tty, ...). On FreeBSD those same routines live in
 * <libutil.h>. launchd.c only uses login_tty(), which FreeBSD's
 * <libutil.h> declares — so this shim is a straight redirect.
 */
#ifndef _FREEBSD_SHIM_UTIL_H_
#define _FREEBSD_SHIM_UTIL_H_

#include <libutil.h>

#endif /* _FREEBSD_SHIM_UTIL_H_ */

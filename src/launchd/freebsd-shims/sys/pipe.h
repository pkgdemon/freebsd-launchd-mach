/*
 * sys/pipe.h — FreeBSD shim for macOS <sys/pipe.h>.
 *
 * On macOS <sys/pipe.h> is a userland-visible header (pipe sizing
 * constants, struct pipe). On FreeBSD <sys/pipe.h> is a *kernel*
 * header — it pulls in vm_page_t / struct selinfo / struct mtx and
 * does not compile in userland.
 *
 * launchd-842's core.c carries a vestigial `#include <sys/pipe.h>`
 * but uses no symbol from it, so this shim is intentionally near
 * empty. BIG_PIPE_SIZE is provided defensively (Apple's value) in
 * case a future code path wants it.
 */
#ifndef _FREEBSD_SHIM_SYS_PIPE_H_
#define _FREEBSD_SHIM_SYS_PIPE_H_

#ifndef BIG_PIPE_SIZE
#define BIG_PIPE_SIZE	(64 * 1024)
#endif

#endif /* _FREEBSD_SHIM_SYS_PIPE_H_ */

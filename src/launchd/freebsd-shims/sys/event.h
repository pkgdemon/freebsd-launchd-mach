/*
 * sys/event.h — FreeBSD shim layered over the base kqueue header.
 *
 * launchd-842's core.c references several macOS kqueue extensions
 * FreeBSD doesn't ship:
 *   - NOTE_EXIT_DETAIL / NOTE_EXITSTATUS / NOTE_EXIT_DECRYPTFAIL /
 *     NOTE_EXIT_MEMORY — EVFILT_PROC sub-flags carrying richer exit
 *     reasons (signal class, jetsam-kill, decrypt failure on iOS).
 *   - EVFILT_MACHPORT — a kqueue filter that wakes when a Mach port
 *     receives a message; the userland half of launchd's MIG server
 *     loop on Apple.
 *
 * FreeBSD's EVFILT_PROC delivers no exit-detail bits and there is no
 * Mach-port kqueue filter. This shim pulls in the base header via
 * #include_next and then appends placeholder values so launchd
 * compiles; the corresponding code paths run but the extra info /
 * the EVFILT_MACHPORT registration is silently inert. A proper port
 * would route Mach-port readiness through libdispatch's existing
 * Mach receive plumbing rather than kqueue.
 */
#ifndef _FREEBSD_SHIM_SYS_EVENT_H_
#define _FREEBSD_SHIM_SYS_EVENT_H_

#include_next <sys/event.h>

/* EVFILT_PROC NOTE_* extensions. Pick bits that don't collide with
 * the FreeBSD-base NOTE_* set. */
#ifndef NOTE_EXITSTATUS
#define NOTE_EXITSTATUS		0x04000000
#endif
#ifndef NOTE_EXIT_DETAIL
#define NOTE_EXIT_DETAIL	0x02000000
#endif
#ifndef NOTE_EXIT_DECRYPTFAIL
#define NOTE_EXIT_DECRYPTFAIL	0x00010000
#endif
#ifndef NOTE_EXIT_MEMORY
#define NOTE_EXIT_MEMORY	0x00020000
#endif
#ifndef NOTE_EXIT_CSERROR
#define NOTE_EXIT_CSERROR	0x00040000
#endif
#ifndef NOTE_EXIT_REPARENTED
#define NOTE_EXIT_REPARENTED	0x00080000
#endif

/*
 * EVFILT_MACHPORT — macOS-only kqueue filter (negative-numbered).
 * FreeBSD uses -1..-13 for its filters; pick -14 so the registration
 * fails cleanly (EINVAL) and core.c's existing error path handles it.
 */
#ifndef EVFILT_MACHPORT
#define EVFILT_MACHPORT		(-14)
#endif

#endif /* _FREEBSD_SHIM_SYS_EVENT_H_ */

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

/* EVFILT_PROC NOTE_* extensions. Apple-only sub-flags carrying
 * richer exit reasons; FreeBSD's EVFILT_PROC accepts only NOTE_EXIT,
 * NOTE_FORK, NOTE_EXEC, NOTE_TRACK, NOTE_TRACKERR, NOTE_CHILD.
 *
 * Earlier this shim picked unique non-zero bits "so they don't
 * collide" with FreeBSD's NOTE_* set. That broke PID-1 launchd
 * boot: launchd's job_start ORs these into proc_fflags and passes
 * them to kevent_mod(EVFILT_PROC). FreeBSD's filt_procattach
 * rejects fflag bits outside NOTE_PCTRLMASK with EINVAL, so
 * kevent_mod returned -1, the parent never called job_uncork_fork(),
 * and the freshly-forked child blocked forever on
 * read(execspair[1]) waiting for the unblock pipe -- getty never
 * exec'd, /dev/console never saw a login prompt.
 *
 * Define them as 0 instead. Two consequences, both correct:
 *   - Read-side checks like `if (kev->fflags & NOTE_EXITSTATUS)`
 *     become always-false, which is the right "FreeBSD's kqueue
 *     doesn't deliver this detail" semantics.
 *   - Write-side OR-ing into proc_fflags adds no bits, so
 *     kevent_mod sees only NOTE_EXIT|NOTE_FORK|NOTE_EXEC -- all
 *     valid on FreeBSD.
 */
#ifndef NOTE_EXITSTATUS
#define NOTE_EXITSTATUS		0
#endif
#ifndef NOTE_EXIT_DETAIL
#define NOTE_EXIT_DETAIL	0
#endif
#ifndef NOTE_EXIT_DECRYPTFAIL
#define NOTE_EXIT_DECRYPTFAIL	0
#endif
#ifndef NOTE_EXIT_MEMORY
#define NOTE_EXIT_MEMORY	0
#endif
#ifndef NOTE_EXIT_CSERROR
#define NOTE_EXIT_CSERROR	0
#endif
#ifndef NOTE_EXIT_REPARENTED
#define NOTE_EXIT_REPARENTED	0
#endif

/*
 * EVFILT_MACHPORT — macOS-only kqueue filter (negative-numbered).
 * FreeBSD uses -1..-13 for its filters; pick -14 so the registration
 * fails cleanly (EINVAL) and core.c's existing error path handles it.
 */
#ifndef EVFILT_MACHPORT
#define EVFILT_MACHPORT		(-14)
#endif

/*
 * EVFILT_TIMER's NOTE_ABSOLUTE on macOS is NOTE_ABSTIME on FreeBSD —
 * launchd-842 uses the macOS spelling.
 */
#ifndef NOTE_ABSOLUTE
#define NOTE_ABSOLUTE		NOTE_ABSTIME
#endif

/*
 * runtime.c's debug-printer FFLAGIF() decodes kqueue fflags into
 * human-readable names. Define macOS-only notes (or notes FreeBSD
 * gates behind __BSD_VISIBLE) as zero — the if (flags & X) probe
 * never matches, so the name simply doesn't appear in the log line,
 * which is the correct degraded behavior.
 */
#ifndef NOTE_REAP
#define NOTE_REAP		0
#endif
#ifndef NOTE_SIGNAL
#define NOTE_SIGNAL		0
#endif

#endif /* _FREEBSD_SHIM_SYS_EVENT_H_ */

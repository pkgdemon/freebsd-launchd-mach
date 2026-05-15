/*
 * os/assumes.h — FreeBSD shim for Apple's os/assumes.h.
 *
 * Apple's os_assumes / os_assumes_zero are "soft assert" macros: they
 * evaluate the expression, and on the failure path log a diagnostic
 * (and on Apple, file a telemetry report) but do NOT abort — they
 * return the value so the caller can keep going. launchd-842's
 * libvproc.c uses os_assumes_zero() to wrap proc_track_dirty() /
 * proc_set_dirty() calls whose failure is non-fatal.
 *
 * Our shim keeps the "evaluate, return the value, don't abort"
 * contract but drops the telemetry. A nonzero result is reported
 * once to stderr so a real failure is still visible. The harder
 * os_assert* (abort-on-failure) variants map to assert().
 */
#ifndef _OS_ASSUMES_H_SHIM_
#define _OS_ASSUMES_H_SHIM_

#include <assert.h>
#include <errno.h>
#include <stdio.h>

/*
 * os_assumes_zero(e) — evaluate e, expect it to be zero. Returns the
 * value of e regardless. Logs once if nonzero. Implemented as a
 * statement-expression so the value propagates and e is evaluated
 * exactly once.
 */
#define os_assumes_zero(e) __extension__({ \
	__typeof__(e) _r = (e); \
	if (_r != 0) { \
		fprintf(stderr, "os_assumes_zero: %s != 0 at %s:%d\n", \
		    #e, __FILE__, __LINE__); \
	} \
	_r; \
})

/* os_assumes(e) — expect e to be true; return e either way. */
#define os_assumes(e) __extension__({ \
	__typeof__(e) _r = (e); \
	if (!_r) { \
		fprintf(stderr, "os_assumes: %s is false at %s:%d\n", \
		    #e, __FILE__, __LINE__); \
	} \
	_r; \
})

/*
 * posix_assumes_zero(e) — the POSIX-call sibling of os_assumes_zero.
 * launchd-842 wraps raw syscalls with it: a return of -1 is the
 * failure signal, so log errno on that path. Returns e unchanged so
 * callers can still inspect the result (many do `... != -1`).
 */
#define posix_assumes_zero(e) __extension__({ \
	__typeof__(e) _r = (e); \
	if (_r == -1) { \
		fprintf(stderr, "posix_assumes_zero: %s == -1 (errno %d) at %s:%d\n", \
		    #e, errno, __FILE__, __LINE__); \
	} \
	_r; \
})

/* os_assert* / posix_assert_zero — hard assert variants. */
#define os_assert(e)		assert(e)
#define os_assert_zero(e)	assert((e) == 0)
#define posix_assert_zero(e)	assert((e) != -1)

/*
 * os_redirect_assumes(fn) — Apple's registration hook that tells the
 * os_assumes_zero failure path to invoke fn() with a formatted error
 * string. Our os_assumes_zero shim writes to stderr directly, so the
 * registration is a no-op. The macro must still expand to a valid
 * file-scope construct (Apple-source uses it like a top-level
 * statement, with a trailing `;`).
 */
#define os_redirect_assumes(fn)	extern int _os_redirect_unused_for_##fn

/*
 * _ctx variants — Apple's os/assumes.h has context-carrying versions
 * that thread a `ctx` cookie through the assumes-redirect callback.
 * Our redirect is unused, so the cookie is dropped.
 */
#define os_assumes_ctx(ctx, e)		os_assumes(e)
#define os_assumes_zero_ctx(ctx, e)	os_assumes_zero(e)
#define posix_assumes_zero_ctx(ctx, e)	posix_assumes_zero(e)

/*
 * __OS_COMPILETIME_ASSERT__(e) — Apple's compile-time assert builtin
 * used like `__OS_COMPILETIME_ASSERT__(sizeof(x) == 4);` at function
 * scope. _Static_assert is the C11 equivalent.
 */
#define __OS_COMPILETIME_ASSERT__(e)	_Static_assert((e), #e)

#endif /* !_OS_ASSUMES_H_SHIM_ */

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

/* os_assert* — hard assert variants. */
#define os_assert(e)		assert(e)
#define os_assert_zero(e)	assert((e) == 0)

#endif /* !_OS_ASSUMES_H_SHIM_ */

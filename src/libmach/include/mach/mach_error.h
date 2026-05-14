/*
 * mach/mach_error.h — human-readable strings for kern_return_t values.
 *
 * Apple's mach_error_string() walks per-subsystem error tables to map
 * a kern_return_t to a descriptive string. We don't ship those
 * tables; for now mach_error_string() is an inline that returns a
 * generic string, and mach_error() prints "<str>: <generic>" to
 * stderr. That's enough for the diagnostic call sites in
 * libbootstrap.c / launchd (which only feed the result to a logger).
 * If precise per-code strings are wanted later, replace the inline
 * with a real table-driven implementation in libsystem_kernel.
 *
 * Without this header, libbootstrap.c's `return mach_error_string(r)`
 * sees an implicitly-declared int-returning function and the compiler
 * errors on the int->const char* conversion.
 */
#ifndef _MACH_MACH_ERROR_H_
#define _MACH_MACH_ERROR_H_

#include <mach/kern_return.h>
#include <stdio.h>

typedef kern_return_t mach_error_t;

static __inline const char *
mach_error_string(mach_error_t error_value)
{
	(void)error_value;
	return "(mach error — see <mach/kern_return.h> for the code)";
}

static __inline void
mach_error(const char *str, mach_error_t error_value)
{
	fprintf(stderr, "%s: %s (0x%x)\n",
	    str ? str : "mach_error",
	    mach_error_string(error_value),
	    (unsigned int)error_value);
}

static __inline const char *
mach_error_type(mach_error_t error_value)
{
	(void)error_value;
	return "(unknown)";
}

#endif /* !_MACH_MACH_ERROR_H_ */

/*
 * mach/mach_time.h — Mach monotonic-clock API. launchd-842's
 * runtime.c uses mach_absolute_time() for its internal timestamps and
 * mach_timebase_info() to convert ticks to nanoseconds.
 *
 * On freebsd-launchd-mach libsystem_kernel implements these over
 * FreeBSD's CLOCK_MONOTONIC: mach_absolute_time() returns nanoseconds
 * from clock_gettime(), so the timebase is 1/1 (numer == denom == 1).
 */
#ifndef _MACH_MACH_TIME_H_
#define _MACH_MACH_TIME_H_

#include <stdint.h>
#include <mach/kern_return.h>

struct mach_timebase_info {
	uint32_t	numer;
	uint32_t	denom;
};
typedef struct mach_timebase_info	*mach_timebase_info_t;
typedef struct mach_timebase_info	mach_timebase_info_data_t;

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t mach_timebase_info(mach_timebase_info_t info);

uint64_t mach_absolute_time(void);
uint64_t mach_continuous_time(void);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_TIME_H_ */

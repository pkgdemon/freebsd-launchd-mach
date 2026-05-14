/*
 * libkern/OSAtomic.h — FreeBSD shim for Apple's atomic-ops header.
 *
 * launchd-842's libvproc.c uses OSAtomicAdd32 for the vproc_t
 * refcount. Apple's OSAtomicAdd32(amount, ptr) atomically adds and
 * returns the NEW value; GCC/clang's __sync_add_and_fetch has exactly
 * those semantics. Only the surface liblaunch actually touches is
 * shimmed here; the full OSAtomic* / OSSpinLock family can be added
 * as the rest of the launchd-842 tree needs it.
 */
#ifndef _LIBKERN_OSATOMIC_H_SHIM_
#define _LIBKERN_OSATOMIC_H_SHIM_

#include <stdint.h>
#include <stdbool.h>

static __inline int32_t
OSAtomicAdd32(int32_t amount, volatile int32_t *value)
{
	return __sync_add_and_fetch(value, amount);
}

static __inline int32_t
OSAtomicIncrement32(volatile int32_t *value)
{
	return __sync_add_and_fetch(value, 1);
}

static __inline int32_t
OSAtomicDecrement32(volatile int32_t *value)
{
	return __sync_sub_and_fetch(value, 1);
}

static __inline bool
OSAtomicCompareAndSwap32(int32_t oldv, int32_t newv, volatile int32_t *value)
{
	return __sync_bool_compare_and_swap(value, oldv, newv);
}

static __inline bool
OSAtomicCompareAndSwapPtr(void *oldv, void *newv, void * volatile *value)
{
	return __sync_bool_compare_and_swap(value, oldv, newv);
}

#endif /* !_LIBKERN_OSATOMIC_H_SHIM_ */

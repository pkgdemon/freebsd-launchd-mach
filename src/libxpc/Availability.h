/*
 * Availability.h — local stub for Apple's <Availability.h>.
 *
 * libxpc's public headers (xpc/base.h, xpc/activity.h, xpc/connection.h,
 * ...) include <Availability.h> and use macros like
 * __OSX_AVAILABLE_STARTING(__MAC_10_9, __IPHONE_7_0). On Apple
 * platforms these expand to compiler attributes that gate the symbol
 * by macOS/iOS version. On FreeBSD they have no meaning — every
 * symbol is unconditionally available — so we expand them to nothing.
 *
 * Reachable via the Makefile's -I${.CURDIR}, the same way uuid/uuid.h
 * is shimmed.
 */
#ifndef _LIBXPC_AVAILABILITY_H_SHIM_
#define _LIBXPC_AVAILABILITY_H_SHIM_

/* All __MAC_* / __IPHONE_* / __OSX_AVAILABLE_* macros expand to
 * nothing — no platform-version gating on FreeBSD. */
#define __MAC_10_0
#define __MAC_10_1
#define __MAC_10_2
#define __MAC_10_3
#define __MAC_10_4
#define __MAC_10_5
#define __MAC_10_6
#define __MAC_10_7
#define __MAC_10_8
#define __MAC_10_9
#define __MAC_10_10
#define __MAC_10_11
#define __MAC_10_12
#define __MAC_10_13
#define __MAC_10_14
#define __MAC_10_15
#define __IPHONE_1_0
#define __IPHONE_2_0
#define __IPHONE_3_0
#define __IPHONE_4_0
#define __IPHONE_5_0
#define __IPHONE_6_0
#define __IPHONE_7_0
#define __IPHONE_8_0
#define __IPHONE_NA

#define __OSX_AVAILABLE_STARTING(...)
#define __OSX_AVAILABLE_BUT_DEPRECATED(...)
#define __OSX_AVAILABLE_BUT_DEPRECATED_MSG(...)
#define __OSX_AVAILABLE(...)
#define __OSX_DEPRECATED(...)
#define __IOS_AVAILABLE(...)
#define __IOS_DEPRECATED(...)
#define __WATCHOS_AVAILABLE(...)
#define __WATCHOS_DEPRECATED(...)
#define __TVOS_AVAILABLE(...)
#define __TVOS_DEPRECATED(...)
#define API_AVAILABLE(...)
#define API_DEPRECATED(...)
#define API_DEPRECATED_WITH_REPLACEMENT(...)
#define API_UNAVAILABLE(...)
#define SPI_AVAILABLE(...)

#endif /* !_LIBXPC_AVAILABILITY_H_SHIM_ */

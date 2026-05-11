/*
 * KMDaemon.h — top-level coordinator for kmodloader.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 freebsd-launchd contributors.
 */

#import <Foundation/Foundation.h>

@interface KMDaemon : NSObject

/* One-shot at boot: ask devmatch(8) which klds are needed for the
 * currently-attached but unattached devices, kldload each. Logs
 * progress via NSLog. Returns when work is done. Phase 2 will swap
 * to a long-running shape with a /dev/devctl source for hot-plug. */
- (void)runOneShot;

@end

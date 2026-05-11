/*
 * main.m — kmodloader daemon entry point (Phase 1 minimal).
 *
 * Phase 1 is a one-shot: enumerate devices via the sysctl dev. tree,
 * match against personalities loaded from /System/Library/Extensions
 * and /Local/Library/Extensions, kldload matching modules, exit.
 *
 * Phase 2 adds DISPATCH_SOURCE_TYPE_READ on /dev/devctl for hot-plug;
 * at that point the daemon stays alive in dispatch_main() and reacts
 * to attach events. For now, kldload at boot is the entire job.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 freebsd-launchd contributors.
 */

#import <Foundation/Foundation.h>
#import "KMDaemon.h"

int
main(int argc, const char *argv[])
{
    (void)argc; (void)argv;

    @autoreleasepool {
        KMDaemon *daemon = [[KMDaemon alloc] init];
        [daemon runOneShot];
    }

    return 0;
}

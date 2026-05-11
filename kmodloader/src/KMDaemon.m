/*
 * KMDaemon.m — top-level coordinator (Phase 1e).
 *
 * Two parallel match paths feed one kldload loop:
 *
 * 1. devmatch(8) — covers any in-tree FreeBSD driver that declares
 *    PNP_INFO. Reads /boot/kernel/linker.hints (and the per-pkg
 *    /boot/modules/linker.hints), walks the live device tree, emits
 *    kld names for unattached devices that have a registered driver.
 *    No flag — bare `devmatch` outputs basename per line.
 *
 * 2. GPU PCI scan — covers drm-kmod GPU drivers (i915kms, amdgpu,
 *    radeonkms) and the nvidia binary blob, none of which declare
 *    PNP_INFO. Reads pciconf -l for class=0x0300xx (display
 *    controllers), then routes per (vendor, device):
 *      0x8086:*       → i915kms
 *      0x1002:<frozen> → radeonkms  (RadeonPCIIDs.h)
 *      0x1002:other   → amdgpu     (default for new AMD silicon)
 *      0x10de:*       → nvidia-drm
 *    Three stable PCI-SIG vendor IDs assigned in the 1980s/90s.
 *    The radeon list is frozen upstream — radeon is in maintenance-
 *    only mode, doesn't gain new device IDs. amdgpu is the open
 *    default that picks up everything new.
 *
 * Hypervisor guest additions (vboxguest, vboxvfs, vmtoolsd-related
 * kmods) are NOT kmodloader's responsibility — they're services,
 * not hardware drivers. The matching launchd plists (planned
 * Phase 2+: org.freebsd.vboxservice.plist, org.freebsd.vmtoolsd.plist)
 * handle their own kldload before launching the userspace daemon,
 * matching FreeBSD's rc.d/vboxservice pattern.
 *
 * Loaded klds are deduped and the GPU drivers run first so the
 * framebuffer / drmn0 is up before peripheral drivers attach.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 freebsd-launchd contributors.
 */

#import "KMDaemon.h"
#import "RadeonPCIIDs.h"

#import <unistd.h>

@interface KMDaemon ()
- (NSArray<NSString *> *)kldsFromDevmatch;
- (NSArray<NSDictionary *> *)gpuDevices;
- (NSArray<NSString *> *)gpuKldsForDevices:(NSArray<NSDictionary *> *)devices;
- (NSSet<NSString *> *)loadedKlds;
- (BOOL)kldload:(NSString *)kld;
@end

@implementation KMDaemon

- (void)runOneShot
{
    NSLog(@"kmodloader: starting (pid=%d)", getpid());

    NSArray<NSDictionary *> *gpuDevices = [self gpuDevices];
    NSArray<NSString *> *gpuKlds = [self gpuKldsForDevices:gpuDevices];
    NSLog(@"kmodloader: %lu GPU device(s) → %lu kld(s)",
          (unsigned long)gpuDevices.count, (unsigned long)gpuKlds.count);

    NSArray<NSString *> *devmatchKlds = [self kldsFromDevmatch];
    NSLog(@"kmodloader: devmatch suggests %lu kld(s)",
          (unsigned long)devmatchKlds.count);

    /* GPU first — once the framebuffer is owned by a DRM driver,
     * peripheral driver loads can't visibly disrupt it. Then the
     * devmatch set fills in the rest. */
    NSMutableOrderedSet<NSString *> *plan = [NSMutableOrderedSet orderedSet];
    [plan addObjectsFromArray:gpuKlds];
    [plan addObjectsFromArray:devmatchKlds];

    NSSet<NSString *> *loaded = [self loadedKlds];
    NSUInteger loadedNow = 0;
    for (NSString *kld in plan) {
        if ([loaded containsObject:kld]) {
            NSLog(@"kmodloader: %@ already loaded; skip", kld);
            continue;
        }
        if ([self kldload:kld]) {
            loadedNow++;
        }
    }

    NSLog(@"kmodloader: done (loaded-now=%lu)", (unsigned long)loadedNow);
}

/* Bare `devmatch` (no flag): output is one kld basename per line for
 * every enabled-but-unattached device that has a registered driver
 * in linker.hints. Strip a .ko suffix defensively (recent versions
 * don't include it but we shouldn't break if a future one does).
 *
 * NOT `devmatch -a`: that flag prints "devname: kld" for every
 * enabled device including already-attached — wrong shape entirely. */
- (NSArray<NSString *> *)kldsFromDevmatch
{
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/sbin/devmatch";
    task.arguments = @[];
    NSPipe *pipe = [NSPipe pipe];
    task.standardOutput = pipe;
    task.standardError = [NSPipe pipe];
    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *exc) {
        NSLog(@"kmodloader: devmatch failed to launch: %@", exc.reason);
        return @[];
    }
    if (task.terminationStatus != 0) {
        NSLog(@"kmodloader: devmatch exited %d", task.terminationStatus);
        return @[];
    }

    NSData *data = [pipe.fileHandleForReading readDataToEndOfFile];
    NSString *output = [[NSString alloc] initWithData:data
                                             encoding:NSUTF8StringEncoding];

    NSMutableOrderedSet<NSString *> *result = [NSMutableOrderedSet orderedSet];
    for (NSString *raw in [output componentsSeparatedByString:@"\n"]) {
        NSString *line = [raw stringByTrimmingCharactersInSet:
                          NSCharacterSet.whitespaceCharacterSet];
        if (line.length == 0) continue;
        if ([line hasSuffix:@".ko"]) {
            line = [line stringByDeletingPathExtension];
        }
        [result addObject:line];
    }
    return result.array;
}

/* Walk pciconf -l for display-controller PCI devices (class 0x0300xx)
 * and return [{"vendor": "0xNNNN", "device": "0xNNNN"}, ...]. AMD
 * picking needs the device ID, not just the vendor — older silicon
 * goes to radeonkms, newer goes to amdgpu (per RadeonPCIIDs.h). */
- (NSArray<NSDictionary *> *)gpuDevices
{
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/usr/sbin/pciconf";
    task.arguments = @[@"-l"];
    NSPipe *pipe = [NSPipe pipe];
    task.standardOutput = pipe;
    task.standardError = [NSPipe pipe];
    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *exc) {
        NSLog(@"kmodloader: pciconf failed to launch: %@", exc.reason);
        return @[];
    }
    if (task.terminationStatus != 0) {
        NSLog(@"kmodloader: pciconf exited %d", task.terminationStatus);
        return @[];
    }

    NSData *data = [pipe.fileHandleForReading readDataToEndOfFile];
    NSString *output = [[NSString alloc] initWithData:data
                                             encoding:NSUTF8StringEncoding];

    NSMutableArray<NSDictionary *> *devices = [NSMutableArray array];
    for (NSString *line in [output componentsSeparatedByString:@"\n"]) {
        /* PCI class 0x0300xx covers VGA controllers (0x030000), 8514
         * (0x030001), XGA (0x030010), 3D controllers (0x030200). */
        if ([line rangeOfString:@"class=0x0300"].location == NSNotFound) {
            continue;
        }
        NSString *vendor = nil, *device = nil;
        NSRange r = [line rangeOfString:@"vendor=0x"];
        if (r.location != NSNotFound &&
            NSMaxRange(r) + 4 <= line.length) {
            vendor = [[line substringWithRange:NSMakeRange(NSMaxRange(r), 4)]
                      lowercaseString];
        }
        r = [line rangeOfString:@"device=0x"];
        if (r.location != NSNotFound &&
            NSMaxRange(r) + 4 <= line.length) {
            device = [[line substringWithRange:NSMakeRange(NSMaxRange(r), 4)]
                      lowercaseString];
        }
        if (!vendor || !device) continue;
        [devices addObject:@{
            @"vendor": [NSString stringWithFormat:@"0x%@", vendor],
            @"device": [NSString stringWithFormat:@"0x%@", device],
        }];
    }
    return devices;
}

- (NSArray<NSString *> *)gpuKldsForDevices:(NSArray<NSDictionary *> *)devices
{
    NSMutableOrderedSet<NSString *> *r = [NSMutableOrderedSet orderedSet];
    for (NSDictionary *dev in devices) {
        NSString *vendor = dev[@"vendor"];
        NSString *device = dev[@"device"];

        if ([vendor isEqualToString:@"0x8086"]) {
            /* Intel: i915kms covers Broadwell onward; older Intel
             * iGPUs are unsupported by drm-kmod and fall back to
             * vgapci/efifb. */
            [r addObject:@"i915kms"];
        }
        else if ([vendor isEqualToString:@"0x1002"]) {
            /* AMD: route per device ID. The radeonkms list is frozen
             * upstream (radeon driver is in maintenance-only mode);
             * everything not in it is amdgpu's job. See RadeonPCIIDs.h. */
            uint16_t devID = (uint16_t)strtoul(
                device.UTF8String, NULL, 16);
            BOOL isRadeon = NO;
            for (size_t i = 0; i < kRadeonKMSDeviceIDsCount; i++) {
                if (kRadeonKMSDeviceIDs[i] == devID) {
                    isRadeon = YES;
                    break;
                }
            }
            [r addObject:isRadeon ? @"radeonkms" : @"amdgpu"];
        }
        else if ([vendor isEqualToString:@"0x10de"]) {
            /* NVIDIA: nvidia-drm depends on nvidia-modeset which
             * depends on nvidia, so kldload of nvidia-drm pulls the
             * chain via MODULE_DEPEND. Requires hw.nvidiadrm.modeset=1
             * set in /boot/loader.conf before this loads (kenv-time
             * tunable). */
            [r addObject:@"nvidia-drm"];
        }
    }
    return r.array;
}

/* Parse kldstat -v for the set of currently-loaded kld basenames. */
- (NSSet<NSString *> *)loadedKlds
{
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/sbin/kldstat";
    task.arguments = @[@"-v"];
    NSPipe *pipe = [NSPipe pipe];
    task.standardOutput = pipe;
    task.standardError = [NSPipe pipe];
    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *exc) {
        NSLog(@"kmodloader: kldstat failed: %@", exc.reason);
        return [NSSet set];
    }

    NSData *data = [pipe.fileHandleForReading readDataToEndOfFile];
    NSString *output = [[NSString alloc] initWithData:data
                                             encoding:NSUTF8StringEncoding];

    NSMutableSet<NSString *> *names = [NSMutableSet set];
    for (NSString *line in [output componentsSeparatedByString:@"\n"]) {
        NSCharacterSet *ws = NSCharacterSet.whitespaceCharacterSet;
        for (NSString *field in [line componentsSeparatedByCharactersInSet:ws]) {
            if ([field hasSuffix:@".ko"]) {
                [names addObject:[field stringByDeletingPathExtension]];
            }
        }
    }
    return names;
}

- (BOOL)kldload:(NSString *)kld
{
    NSLog(@"kmodloader: loading %@", kld);
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/sbin/kldload";
    /* -n: silently succeed if already loaded (defensive against races
     *     between our pre-check and the actual load call). */
    task.arguments = @[@"-n", kld];
    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *exc) {
        NSLog(@"kmodloader: kldload %@ failed to launch: %@", kld, exc.reason);
        return NO;
    }
    if (task.terminationStatus != 0) {
        NSLog(@"kmodloader: kldload %@ exited %d", kld, task.terminationStatus);
        return NO;
    }
    NSLog(@"kmodloader: %@: loaded", kld);
    return YES;
}

@end

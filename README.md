# freebsd-launchd-mach

FreeBSD modernization project. The plan: bring Apple's better lower-level
system services (Mach IPC, launchd, configd, notifyd, asl, libxpc) to
FreeBSD as an out-of-tree kernel module + userland stack, while keeping
GNUstep for the framework layer and FreeBSD ELF for the binary format.
macOS compatibility is a bonus; the goal is a more modern FreeBSD.

- **Plan:** <https://pkgdemon.github.io/freebsd-launchd-mach-plan.html>
- **License:** [BSD-2-Clause](LICENSE) (with per-component Apache 2.0 /
  LGPL / MIT / OSF / CMU headers preserved on imported files &mdash;
  see [NOTICE](NOTICE))
- **Companion:** the [`freebsd-launchd`](https://github.com/pkgdemon/freebsd-launchd)
  repo, the AF_UNIX-only track. This repo is the pure-port track.

## Status

**Phase A** &mdash; *done.* ISO build pipeline lifted from
[`freebsd-launchd`](https://github.com/pkgdemon/freebsd-launchd) works in
CI; live ISO boots stock FreeBSD `init` as PID 1. Curated pkgbase install
(no full `base.txz`/`kernel.txz` extraction) keeps the rootfs lean.

**Phase B** &mdash; *done.* ravynOS's `sys/compat/mach/` ported as an
out-of-tree `mach.ko`. Build gate and runtime gate both met: module
compiles, packages, installs to `/boot/kernel/mach.ko`, and kldloads
cleanly on stock FreeBSD-15.0-RELEASE-p8. Survives fork/exec/exit
stress traffic. ~400 lines of out-of-tree compat shim covering eight
kernel-side deltas (p_machdata aliasing, EVFILT_MACHPORT soft-fail,
Apple syscall arg structs, etc.). Documented at
[`src/mach_kmod/PHASE_B_FINDINGS.md`](src/mach_kmod/PHASE_B_FINDINGS.md).

**Phase B Tier 1** &mdash; *done.* Userland-callable surface added on top
of the kmod:

- **4 wired Mach syscalls** (no-arg trap family): `mach_reply_port`
  (210), `task_self_trap` (211), `thread_self_trap` (212),
  `host_self_trap` (213). Dynamically allocated via
  `kern_syscall_register`; numbers published via
  `sysctl mach.syscall.<name>`.
- **2 live stats sysctls**: `mach.stats.ports_in_use`,
  `mach.stats.kmsgs_in_use` (uma_zone_get_cur over IPC zones).
- **2 in-kernel test sysctls**: `mach.test_port_lifecycle` (port
  alloc/dealloc stress), `mach.test_in_kernel_mqueue` (kmsg/mqueue
  primitive exercise without going through the high-level
  send/receive routing).
- 16-check regression smoke at `src/mach_kmod/smoke-sysctls.sh`.

**Phase C1, C2, C3** &mdash; *done.* Userland Mach bridge:

- **C1: `libmach`** &mdash; userland shared library at
  [`src/libmach/`](src/libmach/) that exposes the wired syscalls as
  named C functions (`mach_reply_port()`, `mach_task_self()`, etc.) and
  `mach_msg()`. Resolves syscall numbers from sysctl on first call.
- **C2: lazy Mach init** &mdash; `mach_task_init_lazy()` and
  `mach_thread_init_lazy()` in the kmod give pre-load processes real
  Mach state on first wired-syscall call instead of `MACH_PORT_NULL`.
- **C3: end-to-end `mach_msg`** &mdash; wired `mach_msg_trap` (syscall
  214) and verified from userland: a test program allocates a reply
  port via libmach, calls `mach_msg(MACH_RCV_MSG | MACH_RCV_TIMEOUT,
  timeout=0)`, kernel returns `MACH_RCV_TIMED_OUT` (0x10004003) as
  expected. Real Mach IPC reachable from userland through libmach.

Total smoke at this checkpoint: **19/19 green** on stock FreeBSD-15
with zero kernel patches.

**Phase D** &mdash; *pkgbase + libsystem layout* &mdash; *done.* Pivoted
away from gershwin-developer's `/System/Library/Libraries/` for our
system libraries. Apple-canonical `libsystem_*` layout under
`/usr/lib/libsystem/` instead, with the
[install layout spike](https://pkgdemon.github.io/freebsd-libxpc-install-layout-spike.html)
as the design doc:

- **Pkgbase migration.** ISO base now installed via curated
  `FreeBSD-*` pkgbase set (`pkglist-base.txt` + `buildpkgs-base.txt`)
  rather than `base.txz`/`kernel.txz` extraction. Lets us omit
  kerberos / lldb / dtrace / audit (runtime) / hyperv-tools /
  bsdinstall / etc. that base.txz had no opt-out for.
- **`libmach` &rarr; `libsystem_kernel`.** Renamed and relocated to
  `/usr/lib/libsystem/libsystem_kernel.so` with `libsystem_kernel.pc`
  pkg-config and headers at `/usr/include/mach/`. ldconfig drop-in at
  `/usr/local/libdata/ldconfig/freebsd-launchd-mach` registers
  `/usr/lib/libsystem` with the runtime linker.
- **`swift-corelibs-libdispatch` vendored** under
  [`src/libdispatch/`](src/libdispatch/), built in our `build.sh`
  chroot pipeline (cmake/ninja). gershwin-developer's FreeBSD
  performance patch (`event_kevent.c` timer fixes +
  `workqueue.c` loop-var typo) applied as a commit on the vendored
  tree. Installs as `/usr/lib/libsystem/libdispatch.so`.
- **`Block.h` + `libBlocksRuntime.so`** ship from libdispatch's bundled
  BlocksRuntime sources (Apple compiler-rt &mdash; same upstream as the
  former `FreeBSD-libblocksruntime` pkg, which is dropped). Single
  source of truth on the system, owned by us.
- **`libsystem_dispatch.so` / `libsystem_blocks.so`** symlinks for
  Apple-canonical link names (`-lsystem_dispatch`).
- **Mach source-type stubs** &mdash; `src/libdispatch/src/event/event_mach_freebsd.c`
  defines `_dispatch_source_type_mach_send/recv` plus the internal
  `_dispatch_mach_type_*` and `_dispatch_xpc_type_sigterm` symbols
  (link-level only; `dispatch_source_create` with a `MACH_*` type
  returns NULL safely). Real polling-thread receive (using
  `mach_msg_trap` from `libsystem_kernel`) is the next session's work.
- **CI smoke** at this checkpoint: 4/4 markers green on the live ISO
  &mdash; `MACH-SMOKE-OK`, `LIBSYSTEM-KERNEL-OK`, `LIBDISPATCH-OK`,
  `LIBDISPATCH-MACH-OK`.

**Next** &mdash; replace the stub `event_mach_freebsd.c` with a real
polling-thread Mach receive implementation (per-source pthread, message
loop calling `mach_msg_trap(MACH_RCV_MSG | MACH_RCV_TIMEOUT)`, hook into
libdispatch's source state machine), then start the libxpc port on top.
Plan docs:

- [**`freebsd-libxpc-plan`**](https://pkgdemon.github.io/freebsd-libxpc-plan.html)
  &mdash; phased porting plan for libxpc on top of mach.ko. Phase 0 was
  the libdispatch Mach backend; subsequent phases fork libxpc from
  ravynOS's tree, add a bootstrap server, swap freebsd-launchd's daemon
  for a clean Apple `launchd-842.92.1` import, build a hybrid
  CoreFoundation library, and finally bring up configd. Also audits the
  related daemons (asl, notifyd, IPConfiguration, mDNSResponder).
- [**`freebsd-libxpc-install-layout-spike`**](https://pkgdemon.github.io/freebsd-libxpc-install-layout-spike.html)
  &mdash; per-component install-path decisions across the four
  candidate layouts (Apple macOS reference, `/usr/lib/libsystem/`,
  gershwin `/System/Library/`, FreeBSD-base `/usr/lib`). Drives the
  Phase D layout above.
- [**`freebsd-libxpc-libdispatch-mach-spike`**](https://pkgdemon.github.io/freebsd-libxpc-libdispatch-mach-spike.html)
  &mdash; libdispatch Mach-backend design + vendoring + build
  integration story.
- [**`freebsd-libxpc-foundation-spike`**](https://pkgdemon.github.io/freebsd-libxpc-foundation-spike.html)
  &mdash; companion spike. Concludes that libgnustep-base alone serves
  every component that needs Foundation, libgnustep-corebase plus a new
  supplementary `libCFRuntime.so` (sourced from swift-corelibs-foundation
  CF + CF-Lite-1153.18 Mach .c files) covers the CoreFoundation needs of
  configd / SystemConfiguration / IPConfiguration. Flags `NWNetworkAgentRegistration`
  (private Apple Network.framework) as the one real Foundation-level
  blocker for configd's IPMonitor plugin.

## Build

CI builds inside a FreeBSD VM via `vmactions/freebsd-vm@v1` and publishes
a continuous release on every push to `main` after the build + boot
smoke-test pass.

To build locally on FreeBSD:

```sh
sh build.sh
```

Produces `out/livecd.iso`.

## Releases

Every push to `main` that passes build + boot test is published as the
continuous release named `FreeBSD-15.0-amd64-mach-YYYYMMDD.iso`.

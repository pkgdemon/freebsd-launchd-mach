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
  returns NULL safely). MACH_RECV gets a real polling-thread
  implementation in Phase E; SEND and the internal types stay as
  link-only stubs.
- **CI smoke** at this checkpoint: 4/4 markers green on the live ISO
  &mdash; `MACH-SMOKE-OK`, `LIBSYSTEM-KERNEL-OK`, `LIBDISPATCH-OK`,
  `LIBDISPATCH-MACH-OK`.

**Phase E** &mdash; *real `DISPATCH_SOURCE_TYPE_MACH_RECV` backend* &mdash;
*done.* The stub `event_mach_freebsd.c` is now a working polling-thread
implementation. `dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV,
port, 0, q)` returns a live source backed by:

- A per-source detached `pthread` that loops on `mach_msg(MACH_RCV_MSG |
  MACH_RCV_LARGE | MACH_RCV_TIMEOUT, rcv_size=0, port, 100ms)`. The
  `MACH_RCV_LARGE` + `rcv_size=0` trick gets `MACH_RCV_TOO_LARGE`
  back from the kernel *without* consuming the message (ravynOS's
  `ipc_mqueue.c` honors the "don't take it off the queue" branch on
  the `LARGE` option). The polling thread fires
  `dispatch_source_merge_data(ds, 1)` on the no-msg&rarr;msg edge so
  libdispatch schedules the user's event handler; the handler reads
  the message itself via `mach_msg(MACH_RCV_MSG, ...)`. Apple's
  documented contract for `MACH_RECV` sources.
- Lifecycle: the thread `_dispatch_retain`s the source as soon as
  `du_owner_wref` is populated, which pins the unote (source dispose
  is what frees the `dispatch_source_refs_s`). The thread exits when
  `dispatch_source_testcancel(ds)` becomes true, then
  `_dispatch_release`s &mdash; breaking the retain so the source / unote
  dealloc normally. No kernel patches; no `HAVE_MACH`; no
  `EVFILT_MACHPORG`.
- libsystem_kernel header (`mach/message.h`) now ships `MACH_RCV_LARGE`,
  `MACH_RCV_TOO_LARGE`, `MACH_MSG_TYPE_MAKE_SEND_ONCE`, and the
  `MACH_MSGH_BITS()` packing macro &mdash; the userland surface needed
  to construct send-once self-messages and run the new round-trip test.
- `test_libdispatch_mach` upgraded from stub-link check to a true
  round-trip: allocates a receive port via `mach_reply_port`, attaches
  a `MACH_RECV` source, sends a self-message via
  `MACH_MSG_TYPE_MAKE_SEND_ONCE`, asserts the handler fires within 5
  seconds and `msgh_id` survives the kernel hop. Same `LIBDISPATCH-MACH-OK`
  CI marker, stronger meaning.
- CI smoke at this checkpoint: still 4/4 markers green on the live ISO.

**Next** &mdash; port libxpc on top of the Phase E foundation. The
ravynOS-fork libxpc (`lib/libxpc/` in their tree) uses a deliberately
small Mach surface that maps cleanly onto what we already have:

- `DISPATCH_SOURCE_TYPE_MACH_RECV` &mdash; the only libdispatch Mach
  source type it touches. Done in Phase E.
- Raw `mach_msg(MACH_SEND_MSG/MACH_RCV_MSG, ...)` via
  libsystem_kernel. Done in Phase C3.
- `mach_port_allocate`, `mach_port_insert_right`,
  `mach_port_deallocate` &mdash; three port-management calls we
  don't have yet. Wire them as new mach.ko-registered syscalls and
  expose them through libsystem_kernel (same pattern as the C1
  `mach_reply_port` family).
- `bootstrap_check_in` / `bootstrap_look_up` against a
  `bootstrap_port` global &mdash; needs a bootstrap server (a small
  daemon that maps service names to mach ports; everything else
  asks it for service ports at connect time).

So the next-step sequencing is:

1. **Three new port-management syscalls** in mach.ko +
   libsystem_kernel (`mach_port_allocate` /
   `mach_port_insert_right` / `mach_port_deallocate`). Small, same
   shape as the four traps we already wired.
2. **Bootstrap server** (`bootstrap_check_in` /
   `bootstrap_look_up`).
3. **Fork ravynOS's `lib/libxpc/` into this repo**, build it
   against our libsystem_kernel + libdispatch + the new
   port-management calls. Smoke marker: `LIBXPC-OK`.

The high-level `dispatch_mach_t` channel API in libdispatch's
`src/mach.c` (~3,250 LOC, gated on `HAVE_MACH`) is *not* on the
critical path &mdash; ravynOS libxpc deliberately reaches for the
lower-level primitives instead. The channel API only comes back if
we later want to support Apple-source consumers that use it directly
(e.g., specific Apple frameworks); none of our current Phase F-G
targets do.

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

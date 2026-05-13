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

**Phase F (port-management syscalls)** &mdash; *done.* Three new
Mach traps wired into mach.ko + exposed through libsystem_kernel,
prerequisites for the ravynOS-fork libxpc and the bootstrap server:

- `mach_port_allocate(task, right, &name)` — allocate a fresh port
  (RECEIVE / SEND / PORT_SET / DEAD_NAME). Syscall 215 on amd64.
- `mach_port_deallocate(task, name)` — drop a send / send-once
  right from the task's namespace. Syscall 216.
- `mach_port_insert_right(task, name, poly, polyPoly)` — attach a
  port right onto an existing name (typically MAKE_SEND on a
  receive-right port; the path libxpc uses for service endpoints).
  Syscall 217.
- All three handlers (`sys__kernelrpc_mach_port_*_trap`) and arg
  structs (`_kernelrpc_mach_port_*_trap_args`) were already
  imported from ravynOS — just needed the
  `mach_syscall_wire.c` registration. NULL-guard wrappers lazy-init
  Mach task state and return `KERN_RESOURCE_SHORTAGE` if init fails.
- New header `mach/mach_port.h` ships `kern_return_t`,
  `mach_port_right_t`, `mach_msg_type_name_t`, and the
  `MACH_PORT_RIGHT_*` / `MACH_MSG_TYPE_*` / `KERN_*` constants
  libxpc uses.
- New smoke test `test_mach_port` does a full allocate &rarr;
  insert_right(MAKE_SEND) &rarr; self-mach_msg(SEND) &rarr;
  mach_msg(RCV) &rarr; deallocate round-trip. CI marker
  `MACH-PORT-OK` between `LIBSYSTEM-KERNEL-OK` and `LIBDISPATCH-OK`.
  5/5 markers green.

**Phase G prereqs (`task_get_special_port` / `task_set_special_port`)**
&mdash; *done.* Two more wired Mach traps that let userland publish
and retrieve `TASK_BOOTSTRAP_PORT` directly (Apple delivers these
over MIG; we expose as syscalls since we don't ship MIG). New
header `mach/task_special_ports.h` with the Apple-canonical
`task_get_bootstrap_port` / `task_set_bootstrap_port` macros. Smoke
marker: `TASK-SPECIAL-PORT-OK`.

**Phase G1 (bootstrap protocol &mdash; single task)** &mdash; *done.*
Hand-rolled message-ID dispatch over Mach IPC (no MIG):

- `bootstrap_check_in(bp, name, *port)` / `bootstrap_look_up(bp,
  name, *port)` matching libxpc's call signatures, declared at
  `/usr/include/servers/bootstrap.h`.
- `bootstrap_server_run(service_port, *stop)` server loop. Receives
  with a 500ms cancel-poll timeout, dispatches on `msgh_id`,
  replies using `MOVE_SEND_ONCE` to the request's `msgh_remote_port`
  (the kernel-manufactured send-once right from the client's
  `MAKE_SEND_ONCE`).
- Service map is a 64-slot static array in the server's address
  space &mdash; same-task scope only. Cross-process scope (Phase G2)
  lifts this to a real daemon + complex-message port-descriptor
  transfer.
- Smoke marker `BOOTSTRAP-OK`. Test allocates a service port,
  publishes it via `task_set_bootstrap_port`, spawns a pthread
  running `bootstrap_server_run`, calls `bootstrap_check_in` then
  `bootstrap_look_up` for the same name from the main thread, asserts
  the returned port names match.

**Phase G2a (complex Mach messages on the bootstrap wire)** &mdash;
*done.* Reply upgraded to a complex message
(`MACH_MSGH_BITS_COMPLEX` + `mach_msg_body_t` +
`mach_msg_port_descriptor_t` + result) so the kernel translates
the port reference between sender and receiver IPC spaces &mdash;
prerequisite for cross-task port-right transfer. Bug hunt during
validation exposed and fixed a Phase G1 false positive where the
combined `MACH_SEND_MSG | MACH_RCV_MSG` form reused the buffer for
both directions: the kernel wrote the reply into the same memory
that held the request, and the test was vacuously comparing
`0 == 0`. Now uses split `mach_msg(SEND)` + `mach_msg(RCV)` and
asserts non-`MACH_PORT_NULL` ports in the test.

**Phase G2b (host-bootstrap-port fallback)** &mdash; *done.* mach.ko
gained a HOST_BOOTSTRAP_PORT slot in `realhost.special[]`;
`task_get_special_port(TASK_BOOTSTRAP_PORT)` falls back to it when
the per-task `itk_bootstrap` slot is null. Bootstrap server calls
`host_set_special_port(HOST_BOOTSTRAP_PORT, port)` once at startup
to publish its port host-wide; every other process discovers it via
`task_get_bootstrap_port`. Smoke marker: `HOST-BOOTSTRAP-OK`.

This phase also forced an architectural pivot that affects everything
going forward:

**Mach trap multiplexer** &mdash; *done.* FreeBSD's `syscalls.master`
reserves exactly 10 dynamic syscall slots (`lkmnosys` at 210-219).
We&rsquo;ve used all 10 on foundational Mach traps; the
launchd+configd+notifyd+asl port wants 20-30 more. FreeBSD&rsquo;s
`RESERVED` slots (~48 of them) *cannot* be claimed via
`kern_syscall_register` &mdash; the kernel only accepts slots whose
`sy_call` is `lkmnosys` or `lkmressys`, and RESERVED slots use plain
`nosys`. So we built a multiplexer dispatcher:

- One FreeBSD syscall slot (219) hosts `sys_mach_trap_mux_trap`,
  which switches on an op-number argument and dispatches to the
  underlying Mach trap handler internally. Architecturally
  identical to Apple&rsquo;s `mach_trap_table` &mdash; just routed
  at the C handler level instead of in the assembly trampoline.
- Op-numbers defined in `<mach/mach_traps_mux.h>`:
  - `1` &nbsp;`host_set_special_port`
  - `2` &nbsp;`task_set_special_port` (migrated off its former
    dedicated slot 219 to free it for the multiplexer)
- Future Mach traps (`mach_port_mod_refs`,
  `mach_port_request_notification`, `mach_port_get_attributes`,
  `task_threads`, the rest of the `mach_port_*member` family,
  `vm_*`, `semaphore_*`, etc.) drop in as new op-numbers without
  consuming any further FreeBSD syscall slots. The 9 hot dedicated
  slots stay put because either they're called frequently
  (`mach_msg_trap`, `mach_reply_port`, the `_self_trap` family) or
  ABI-constrained (`mach_msg_trap` has a 7-arg shape that exceeds
  the multiplexer&rsquo;s 5-arg budget).
- Per-call overhead is ~5ns of dispatch (switch + function call) &mdash;
  comfortably under 1% of trap-entry cost. Apple&rsquo;s
  `mach_trap_table` validates the architectural pattern at scale.

Audit plan documenting the trade space:
[freebsd-mach-kmod-syscall-slots-spike](https://pkgdemon.github.io/freebsd-mach-kmod-syscall-slots-spike.html).

**Next (Phase G2: complete the bootstrap server)** &mdash; two
remaining steps:

1. **G2c: standalone `bootstrap_server` daemon binary.** Allocates
   its service port, calls `host_set_special_port`, enters the
   existing `bootstrap_server_run` loop. Installed but *not* wired
   into `rc.local`: the bootstrap server's real home is as a
   launchd-managed job, which only happens once launchd is PID 1
   (a much later phase). Until then, the daemon is ephemeral &mdash;
   started by the test harness, killed after.
2. **G2d: cross-process test.** `run.sh` launches the daemon in
   the background, runs `test_bootstrap_remote` (a `fork`-based
   client that calls `task_get_bootstrap_port` and exercises
   check_in / look_up against the running daemon), kills the
   daemon afterward. New smoke marker: `BOOTSTRAP-REMOTE-OK`.

After Phase G2:

- **Fork ravynOS's `lib/libxpc/`** into this repo, build it against
  our libsystem_kernel + libdispatch + bootstrap server. Smoke
  marker: `LIBXPC-OK`.

The high-level `dispatch_mach_t` channel API in libdispatch's
`src/mach.c` (~3,250 LOC, gated on `HAVE_MACH`) is *not* on the
critical path &mdash; ravynOS libxpc deliberately reaches for the
lower-level primitives we now have. The channel API only comes back
if we later want to support Apple-source consumers that use it
directly (e.g., specific Apple frameworks); none of our current
Phase F-G targets do.

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

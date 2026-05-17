# Vendored: Apple libnotify + notifyd

**Upstream:** https://github.com/apple-oss-distributions/Libnotify
**Upstream commit:** `715d461778f6b93c821d99390a0078bd6f6d8c04`
**Imported:** 2026-05-16 (Phase J0 of the ASL port — bundled because
libnotify is the highest-leverage cross-cutting dep alongside ASL)
**License:** APSL 2.0 (see `APPLE_LICENSE`)

## Why we're here

Apple's libnotify provides a Mach-backed pub/sub notification bus. Used
by every Apple-source service daemon:

| Consumer | `notify_*` call sites | Files |
|---|---|---|
| ASL (syslogd + libsystem_asl) | 52 | 9 |
| IOKitUser (pwr_mgt + ps) | 56 | 11 |
| configd | 60 | 14 |
| IPConfiguration | 7 | 1 |
| PowerManagement (pmconfigd) | 154 | 17 |
| DiskArbitration | 6 | 2 |
| **Total** | **335** (+12 in syslog tests) | **54** |

Stubbing was considered but **configd and pmconfigd are event-driven**
— they poll `notify_check()` for system-wide events (network change,
power state). With stubs they'd never see changes and run permanently
frozen. One ~12.8k-LoC libnotify port unblocks 5 daemon ports cleanly
vs ~480 lines of degraded-mode shim across them.

## What we got

- Top-level: `libnotify.c` (1228 LoC), `libnotify.h`, `notify_client.c`
  (4752 LoC), `notify_internal.h`, plus public headers and man pages.
- `notifyd/` subdir: the daemon. `notifyd.c` (1488), `notify_proc.c`
  (2070), `pathwatch.c` (955), `service.c` (271).
- `notify_ipc.defs` — MIG IPC subsystem.

Total: ~12.8k LoC non-test.

## Porting plan

Per https://pkgdemon.github.io/freebsd-asl-plan.html, libnotify is
ported in lockstep with ASL:

- **J0**: vendor source (this commit)
- **J1**: build `libnotify.so` (compile-only); MIG `notify_ipc.defs`
  via in-tree `migcom`. Install to `/usr/lib/system/`.
- **J2**: build `notifyd` daemon, register Mach service
  `com.apple.system.notification_center`, run under PID-1 launchd via
  plist. Smoke: client A posts; client B (registered watcher) receives
  via Mach.

## Cross-deps

- `libxpc` (done), `libdispatch` (done) — both used by notifyd
- Mach IPC via mach.ko (done)
- bootstrap server (done — `com.apple.bootstrap_server`)
- FreeBSD-native `libbsm` — used for audit_token-based access control
- `sys/fileport.h` — Mach-port-wraps-fd syscalls. **Not yet
  implemented in mach.ko**. Both notifyd and ASL `dbserver.c:36,1577`
  need these. Open question per the plan: add to mach.ko trap
  multiplexer, or userland-only emulate via per-fd Mach ports.
- ASL — notifyd logs via `asl_log()`. Circular dep with ASL; resolve
  by using `syslog(3)` during notifyd bootstrap, switching to
  `asl_log()` once libsystem_asl loads.

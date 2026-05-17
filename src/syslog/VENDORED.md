# Vendored: Apple syslog (ASL)

**Upstream:** https://github.com/apple-oss-distributions/syslog
**Upstream commit:** `69d8586e9a599c304e1b702a9c2830e2f78126d5`
**Imported:** 2026-05-16 (Phase J0 of the ASL port)
**License:** APSL 2.0 (see `APPLE_LICENSE`)

## Why we're here

Apple's syslog repository contains the full ASL (Apple System Log) stack:
- `libsystem_asl.tproj/` — client library (`libsystem_asl.so`); every
  Apple-source daemon links this and calls `asl_log()` / `asl_send()`
- `syslogd.tproj/` — the syslog daemon; replaces the removed
  FreeBSD-syslogd (dropped from `pkglist-base.txt` at commit `88694f0`)
- `aslcommon/` — shared static lib + MIG `asl_ipc.defs` subsystem 114
- `aslmanager.tproj/` — log-rotation daemon (Phase J4 — ships
  alongside syslogd because without it `/var/log/asl/` grows
  unbounded)
- `util.tproj/` — `syslog(1)` CLI (Phase J5, deferred —
  `tail /var/log/messages` works via BSD output route, syslog(1)
  is operator-convenience for ASL DB queries / runtime filter
  control)
- `newsyslog/` — BSD-style log rotation (SKIP: FreeBSD base has its own)

## Porting plan

Per https://pkgdemon.github.io/freebsd-asl-plan.html:
- **J0**: vendor source (this commit)
- **J1**: build `libsystem_asl.so` (compile-only)
- **J2**: build syslogd, Mach RX loop only; first messages persist to
  `/var/log/asl/`
- **J3**: add `bsd_in.c` module + RFC 5424 parser branch + launchd
  Sockets entry → FreeBSD getty/login `syslog(3)` transparently
  reaches ASL
- **J4**: hwregd swap stderr → asl_log()
- **J5** (deferred): aslmanager + `syslog(1)` CLI

## Patches we expect to apply

1. **New file `syslogd.tproj/bsd_in.c`** — modeled on `klog_in.c`;
   listens on `/var/run/log` (AF_UNIX SOCK_DGRAM) so FreeBSD-base
   `syslog(3)` consumers reach ASL transparently.
2. **`syslogd.tproj/daemon.c:1057`** — add RFC 5424 branch in
   `asl_syslog_input_convert()`. FreeBSD libc emits RFC 5424
   (`<pri>1 timestamp ...`); Apple's existing parser hard-checks for
   RFC 3164 (`Mmm dd HH:MM:SS`). Both must coexist.
3. **`syslogd.tproj/com.apple.syslogd.plist`** — add a `Sockets`
   entry for `/var/run/log` so launchd creates and hands the dgram
   socket to syslogd at check-in.
4. **Build system** — Apple ships Xcode project; we build via
   `bsd.lib.mk` / `bsd.prog.mk` Makefiles mirroring our libxpc /
   libdispatch / libCoreFoundation setup.

## Cross-cutting deps we ship alongside

- `src/Libnotify/` — Apple libnotify + notifyd. **Hard prerequisite**
  for the downstream daemons (configd 60 sites, PowerManagement
  154 sites, IOKitUser 56 sites) that hard-depend on real notify
  behavior. Stubbing would leave them permanently broken; one
  ~12.8k-LoC libnotify port unblocks 5 daemon ports cleanly.
- Compile-gate stubs under `src/freebsd-shims/`:
  - `os/log.h` + `os/log_private.h` + `os/assumes.h` +
    `os/activity.h` + `os/transaction_private.h` (~700 lines)
  - `membership.h` (~20 lines, `mbr_uid_to_uuid` → KERN_FAILURE)
  - `configuration_profile.h` (~10 lines, MDM-profile no-op)
- Use FreeBSD base `libbsm` as-is — identical
  `audit_token_to_au32` / `audit_token_to_pid` signatures in
  `contrib/openbsm/bsm/libbsm.h:1257-1294`.

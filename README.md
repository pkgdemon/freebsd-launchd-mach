# freebsd-launchd-mach

FreeBSD modernization project. The plan: bring Apple's better lower-level
system services (Mach IPC, launchd, configd, notifyd, asl, libdispatch,
libxpc) to FreeBSD as an out-of-tree kernel module + userland stack,
while keeping GNUstep for the framework layer and FreeBSD ELF for the
binary format. macOS compatibility is a bonus; the goal is a more
modern FreeBSD.

- **Plan:** <https://pkgdemon.github.io/freebsd-launchd-mach-plan.html>
- **License:** [BSD-2-Clause](LICENSE) (with per-component Apache 2.0 /
  OSF / CMU headers preserved on imported files &mdash; see [NOTICE](NOTICE))
- **Companion:** the [`freebsd-launchd`](https://github.com/pkgdemon/freebsd-launchd)
  repo, the AF_UNIX-only track. This repo is the pure-port track.

## Status

**Phase A** &mdash; pipeline scaffold lifted from
[`freebsd-launchd`](https://github.com/pkgdemon/freebsd-launchd). At this
stage the build produces a vanilla FreeBSD live ISO that boots with
stock FreeBSD `init` as PID 1. No Mach kernel module yet, no launchd,
no configd. Phase A's only purpose is proving the new repo's build
pipeline works in CI before any Mach work begins.

**Phase B** (next, ~3 weeks) &mdash; port ravynOS's `sys/compat/mach/` as
an out-of-tree `mach.ko`. Build it in CI, install to `/boot/kernel/mach.ko`
on the live ISO, set `loader.conf: mach_load="YES"` so loader preloads
it before `init` runs. Userland smoke test does a `mach_port_allocate` +
`mach_msg(SEND|RCV)` round-trip via the syscalls `mach.ko` registers.

After Phase B: **hard stop**. Discuss libxpc choice, libdispatch choice,
plist format, and whether to proceed to Phase C+ (userland Mach stack,
Apple's launchd, configd, IPConfiguration). See the plan for details.

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

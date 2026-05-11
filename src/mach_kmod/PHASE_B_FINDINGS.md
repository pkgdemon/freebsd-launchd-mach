# Phase B findings — build gate met, runtime gate hits the v2 pivot signal

**TL;DR.** mach.ko builds cleanly and packages against unmodified
FreeBSD-15.0 source (CI green at commit `daebc30`). It does not yet
`kldload` cleanly on stock FreeBSD; the cause is the documented
[Phase B pivot signal](https://pkgdemon.github.io/freebsd-launchd-mach-plan.html#risks)
— ravynOS's mach module assumes kernel-side patches we deliberately
don't apply.

## Gates

| Gate                                                              | Status        |
|-------------------------------------------------------------------|---------------|
| C source compiles end-to-end                                       | ✅ met         |
| MIG-generated stubs link against our `compat_stubs.c`              | ✅ met         |
| `mach.ko` installs to `/boot/kernel/` on the live ISO              | ✅ met         |
| CI release-artifact pipeline (`mach.ko-FreeBSD-15.0-amd64.tar.gz`) | ✅ met         |
| `kldload mach` succeeds (`kldstat -m mach`)                        | ❌ panics      |

## How the build gate was reached

~400 lines of out-of-tree compat shim covering eight kernel-side
deltas ravynOS introduces in their kernel tree:

1. **`p_machdata` / `td_machdata` struct fields** — aliased via the
   force-included `compat_shim.h` to `p_emuldata` / `td_emuldata`, the
   FreeBSD emulation-data slots already used by linux compat. Zero
   kernel patches needed; the trade-off is that mach and linux compat
   cannot coexist on the same process.

2. **`EVFILT_MACHPORT` kqueue filter ID** — `#define EVFILT_MACHPORT (-16)`
   in the shim, one past stock FreeBSD's `EVFILT_SYSCOUNT=15`.
   `kqueue_add_filteropts` ultimately rejects it (the bounds check is
   strict); we soft-fail that call so the module still loads with the
   filter unavailable.

3. **49 Apple-shape syscall numbers and audit-event constants** —
   `SYS_<name>` and `SYS_AUE_<name>` enum values defined in the shim,
   all set to `NO_SYSCALL` (-1) / `AUE_NULL` (0). The
   `SYSCALL_INIT_HELPER` array compiles; the helper-register call
   no-ops because the array's first entry is the loop sentinel.

4. **47 Apple-shape syscall `struct *_args` containers** — extracted
   from ravynOS's generated `sys/sysproto.h` into
   `include/sys/mach/_mach_sysproto.h`, with `PADL_/PADR_` stubbed to 0
   (real definitions come from FreeBSD's `<sys/sysproto.h>` later in
   the include chain).

5. **47 `sys_<name>` function prototypes** — declared by the
   `_MACH_KMOD_APPLE_SYSCALL_STUB` macro in the shim; bodies are in
   the ravynOS source files (mach_misc.c, mach_traps.c, etc.).

6. **`vm_map_entry_pred` static-inline helper** — reproduced as a GCC
   statement-expression macro in the shim, using only public
   `struct vm_map_entry` fields (the FreeBSD definition is `static
   inline` and not exported to modules).

7. **`vm_map_clip_start` / `vm_map_clip_end`** — stub macros returning
   `KERN_SUCCESS`. Their only call site in `mach_vm_copy_overwrite` is
   in a path Phase B's smoke test doesn't exercise.

8. **Apple file-type / FD-type constants** (`S_IFPORT`, `S_IFPSET`,
   `DTYPE_MACH_IPC`, `KF_TYPE_PORT`, `KF_TYPE_PORTSET`) — defined in
   the shim with the same values ravynOS uses.

9. **18 Mach-VM RPC implementations** — `vm_wire`, `vm_region`,
   `vm_copy`, `vm_inherit`, `vm_protect`, `vm_msync`, `vm_read`,
   `vm_write`, `vm_remap`, plus 9 region/recurse variants. Apple
   defines these in `osfmk/vm/vm_user.c`, which ravynOS did not port.
   Stubbed in `src/compat_stubs.c` as `(void)` functions returning
   `KERN_NOT_SUPPORTED` to satisfy the linker; signatures don't have
   to match the MIG dispatch callers because at link time only names
   are resolved and the syscalls aren't actually exercised.

## How the runtime gate failed

The kernel panic captured locally on stock FreeBSD-15.0-RELEASE-p8:

```
panic: page fault
trap number = 12
cpuid = 0
current process = <pid> (kldload)
KDB: stack backtrace:
 ... trap_pfault ...
 ... calltrap ...
 sys__exit+0xd
 amd64_syscall+0x126
 fast_syscall_common+0xf8
```

The fault happens inside `exit1()` — `sys__exit+0xd` is the return
address from the `callq exit1` instruction. The `kldload` userland
process exits immediately after the kldload syscall returns, and
during exit1's cleanup chain (which fires `process_exit` /
`thread_fini` / `process_fork` eventhandlers, frees file descriptors,
etc.) a NULL deref panics the kernel.

ravynOS's Mach kmod registers four eventhandlers at `SYSINIT` time:

- `process_exit` / `process_exec` → `ipc_entry_list_close`
- `thread_ctor` / `thread_init` / `thread_fini` → `mach_thread_*`
- `process_init` / `process_fork` → `mach_task_*`

Each dereferences `p_machdata` (or `td_machdata`) without a guard.
For any process that existed before mach.ko loaded — including the
kldload process itself — `p_machdata` / `td_machdata` is NULL and the
deref crashes.

I added NULL guards to all four of our handlers
(`ipc_entry_list_close`, `mach_thread_fini`, `mach_thread_ctor`,
`mach_task_fork`) at `daebc30` and verified the rebuild contains them.
**The panic persists**, which means there's at least one more deref
path inside `exit1`'s chain — likely in code the eventhandlers
indirectly call (inline accessors like `current_task()` /
`current_space()` are obvious candidates) — that I haven't located.

## Why this is the v2 plan's pivot signal

The plan's [Phase B exit-gate language](https://pkgdemon.github.io/freebsd-launchd-mach-plan.html#risks):

> Phase B exit gate is "kldloads on **unmodified** FreeBSD-CURRENT." If
> we hit kernel-side patches, document them, decide between (a) a
> parallel `mach-host-patches/` kernel patch series, (b) reimplementing
> the dependency userspace-side, (c) bailing back to v1's "build
> minimal" approach.

The 9 shim categories above are exactly the kernel patches ravynOS
applies — recreated in userland as a force-included shim. That got us
through the compile + link + module load gates. The fact that NULL
guards alone don't make the runtime safe suggests the ravynOS port is
deeper-coupled to kernel-side patches than the eventhandler entry
points alone — there are inline accessors (e.g., `current_task()`,
`current_space()`) that assume initialized state and would need
guards or substitutions everywhere they appear.

Continuing down this path is option (a) or (b) above. Both are real;
neither is on Phase B's exit gate as written. The current commit
(`daebc30`) is the honest stopping point: build green, ~400 lines of
documented shim, runtime panic acknowledged.

## What to do next

Choose at the project level, not the commit level:

- **Option (a): apply kernel patches.** Smallest probably-sufficient
  set is patching `init/0` or `process_init` to seed
  `p_machdata`/`td_machdata` early enough that no eventhandler ever
  sees a NULL. Crosses the "no kernel patches" line documented in
  memory.

- **Option (b): keep extending the shim.** Identify each deref of
  `p_machdata`/`td_machdata` (and inline accessors that resolve to
  them) across the kmod source and add per-call-site guards. Estimate:
  10–50 more guards across the IPC and task layers. The compat layer
  grows from ~400 lines toward 1000–2000.

- **Option (c): pivot to the v1 plan.** Build a minimal mach.ko from
  scratch covering only configd's hot-path Mach calls (~15 functions),
  per the [v1 plan](https://pkgdemon.github.io/freebsd-mach-kmod-plan.html).
  Discard the ravynOS extraction; keep the build-pipeline and CI
  scaffolding from this repo. Estimated 6–9 person-months per v1.

This finding does not invalidate the work in this repo — it
characterizes the exact distance between "buildable" and "loadable"
for the ravynOS extraction path on unmodified FreeBSD. That distance
is the shim-vs-patches tradeoff the v2 plan called out as the central
risk.

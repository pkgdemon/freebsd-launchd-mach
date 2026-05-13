#!/bin/sh
# /usr/tests/freebsd-launchd-mach/run.sh — phase B kernel-side smoke
# check.
#
# Invoked by CI's boot-test.sh expect script after root login. Prints
# MACH-SMOKE-OK / MACH-SMOKE-FAIL on a single line so CI can match
# either marker exactly.
#
# Path follows FreeBSD convention: base-system tests live under
# /usr/tests/<component>/. We don't (yet) use ATF/Kyua framework here
# — a plain shell script is enough for one trivial smoke check. When
# the suite grows (libmach roundtrip, mach_msg, port allocation,
# etc.), revisit and adopt atf-sh + Kyuafile so `kyua test` works.

set -u

# 1. kernel-side: mach.ko loaded.
if kldstat -m mach >/dev/null 2>&1; then
    echo "MACH-SMOKE-OK: mach.ko is loaded"
    kldstat -v 2>/dev/null | grep -i mach || true
else
    echo "MACH-SMOKE-FAIL: mach.ko is NOT loaded"
    echo "kldstat output:"
    kldstat
    exit 1
fi

# 2. userland: libsystem_kernel resolves and Mach traps roundtrip.
# Proves the migrated /usr/lib/libsystem/libsystem_kernel.so is
# discoverable via rtld, links into the test binary, and its
# Mach-trap calls actually return valid ports (post Phase C2 lazy
# init, all four traps must succeed).
if [ -x /usr/tests/freebsd-launchd-mach/test_libmach ]; then
    if /usr/tests/freebsd-launchd-mach/test_libmach; then
        echo "LIBSYSTEM-KERNEL-OK: libsystem_kernel roundtrip succeeded"
    else
        rc=$?
        echo "LIBSYSTEM-KERNEL-FAIL: test_libmach exit=$rc"
        echo "ldd:"
        ldd /usr/tests/freebsd-launchd-mach/test_libmach 2>&1 || true
        exit 1
    fi
else
    echo "LIBSYSTEM-KERNEL-FAIL: test_libmach binary not installed"
    exit 1
fi

# 2b. userland: mach_port_allocate / _insert_right / _deallocate
# traps. These are the three new syscalls Phase F-prep wired so the
# ravynOS-fork libxpc can allocate Mach ports beyond the
# task/thread/host/reply family. The test allocates a receive-right
# port, attaches a send right onto the same name, sends and drains a
# self-message through it, then drops the send right — full
# allocate-use-deallocate round-trip.
if [ -x /usr/tests/freebsd-launchd-mach/test_mach_port ]; then
    if /usr/tests/freebsd-launchd-mach/test_mach_port; then
        echo "MACH-PORT-OK: mach_port_* round-trip succeeded"
    else
        rc=$?
        echo "MACH-PORT-FAIL: test_mach_port exit=$rc"
        exit 1
    fi
else
    echo "MACH-PORT-FAIL: test_mach_port binary not installed"
    exit 1
fi

# 2c. userland: task_get_special_port / task_set_special_port. Phase G
# prerequisite — the bootstrap server uses task_set_bootstrap_port on
# each client task to publish its receive port, and clients read it
# back via task_get_bootstrap_port at mach_init time. The test mints
# a service-shaped port, stores it as TASK_BOOTSTRAP_PORT, reads it
# back, and asserts the round-trip preserves the port name.
if [ -x /usr/tests/freebsd-launchd-mach/test_task_special_port ]; then
    if /usr/tests/freebsd-launchd-mach/test_task_special_port; then
        echo "TASK-SPECIAL-PORT-OK: TASK_BOOTSTRAP_PORT set/get round-trip succeeded"
    else
        rc=$?
        echo "TASK-SPECIAL-PORT-FAIL: test_task_special_port exit=$rc"
        exit 1
    fi
else
    echo "TASK-SPECIAL-PORT-FAIL: test_task_special_port binary not installed"
    exit 1
fi

# 2d. userland: bootstrap protocol round-trip (Phase G1). Hand-rolled
# message-ID server loop dispatching CHECK_IN / LOOK_UP requests over
# Mach IPC. The test spawns a pthread that runs bootstrap_server_run,
# then from the main thread does check_in("com.example.test") followed
# by look_up of the same name and asserts the returned port matches.
# Single-task only — cross-process needs complex-message port
# descriptors, lands in Phase G2 alongside the daemon.
if [ -x /usr/tests/freebsd-launchd-mach/test_bootstrap ]; then
    if /usr/tests/freebsd-launchd-mach/test_bootstrap; then
        echo "BOOTSTRAP-OK: bootstrap protocol round-trip succeeded"
    else
        rc=$?
        echo "BOOTSTRAP-FAIL: test_bootstrap exit=$rc"
        exit 1
    fi
else
    echo "BOOTSTRAP-FAIL: test_bootstrap binary not installed"
    exit 1
fi

# 3. userland: libdispatch loads + serial queue executes a sync callback.
# Baseline check that the vendored swift-corelibs-libdispatch (built
# in our chroot pipeline, installed to /usr/lib/libsystem/) is loadable
# via rtld and dispatches a function-pointer callback correctly. The
# Mach IPC backend test (DISPATCH_SOURCE_TYPE_MACH_RECV) lands in a
# follow-up commit once event_mach_freebsd.c is wired in.
if [ -x /usr/tests/freebsd-launchd-mach/test_libdispatch ]; then
    if /usr/tests/freebsd-launchd-mach/test_libdispatch; then
        echo "LIBDISPATCH-OK: libdispatch baseline roundtrip succeeded"
    else
        rc=$?
        echo "LIBDISPATCH-FAIL: test_libdispatch exit=$rc"
        echo "ldd:"
        ldd /usr/tests/freebsd-launchd-mach/test_libdispatch 2>&1 || true
        exit 1
    fi
else
    echo "LIBDISPATCH-FAIL: test_libdispatch binary not installed"
    exit 1
fi

# 4. Mach IPC backend round-trip: DISPATCH_SOURCE_TYPE_MACH_RECV with
# the real polling-thread backend (event_mach_freebsd.c) — allocate a
# port via mach_reply_port, attach a dispatch source, self-send a
# message, verify the handler fires within 5s and consumes it. Proves:
# event_mach_freebsd.c spawns a working poll thread; mach_msg(MACH_RCV_
# LARGE, rcv_size=0) peek path returns TOO_LARGE without consuming;
# dispatch_source_merge_data wakes the handler; handler's mach_msg(
# MACH_RCV_MSG) drains the message; clean cancel/release teardown.
if [ -x /usr/tests/freebsd-launchd-mach/test_libdispatch_mach ]; then
    if /usr/tests/freebsd-launchd-mach/test_libdispatch_mach; then
        echo "LIBDISPATCH-MACH-OK: Mach RECV round-trip succeeded"
    else
        rc=$?
        echo "LIBDISPATCH-MACH-FAIL: test_libdispatch_mach exit=$rc"
        exit 1
    fi
else
    echo "LIBDISPATCH-MACH-FAIL: test_libdispatch_mach binary not installed"
    exit 1
fi

exit 0

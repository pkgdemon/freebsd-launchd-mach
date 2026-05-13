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

exit 0

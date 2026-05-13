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

if kldstat -m mach >/dev/null 2>&1; then
    echo "MACH-SMOKE-OK: mach.ko is loaded"
    kldstat -v 2>/dev/null | grep -i mach || true
    exit 0
else
    echo "MACH-SMOKE-FAIL: mach.ko is NOT loaded"
    echo "kldstat output:"
    kldstat
    exit 1
fi

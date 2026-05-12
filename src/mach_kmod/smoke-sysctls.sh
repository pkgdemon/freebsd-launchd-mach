#!/bin/sh
# Phase B regression smoke: hit every mach.* sysctl + each wired syscall.
# Usage: ./smoke-sysctls.sh   (run as root on the FreeBSD host with mach.ko loaded)
# Output: per-test PASS/FAIL line; non-zero exit if anything fails.
set -u
PASS=0; FAIL=0

check() {
    name=$1; shift
    if "$@" >/tmp/smk.out 2>&1; then
        echo "PASS $name: $(head -1 /tmp/smk.out)"
        PASS=$((PASS+1))
    else
        echo "FAIL $name: $(head -1 /tmp/smk.out)"
        FAIL=$((FAIL+1))
    fi
}

# Build test helpers if not present. Sources travel with the repo.
HERE="$(dirname "$0")"
LIBMACH_DIR="$HERE/../libmach"
TEST_BIN=/tmp/test_mach_syscall0
TEST_SRC="$HERE/tests/test_mach_syscall0.c"
TEST_LIBMACH_BIN=/tmp/test_libmach
TEST_LIBMACH_SRC="$HERE/tests/test_libmach.c"
TEST_MACH_MSG_BIN=/tmp/test_mach_msg
TEST_MACH_MSG_SRC="$HERE/tests/test_mach_msg.c"

if [ ! -x "$TEST_BIN" ] || [ "$TEST_SRC" -nt "$TEST_BIN" ]; then
    cc -O2 -o "$TEST_BIN" "$TEST_SRC" || {
        echo "FAIL build test_mach_syscall0"
        FAIL=$((FAIL+1))
    }
fi

# Build test_libmach by linking the libmach source directly. This avoids
# needing a shared library install for smoke iteration; the actual .so
# is produced by `make -C $LIBMACH_DIR` for shipping.
if [ ! -x "$TEST_LIBMACH_BIN" ] || \
   [ "$TEST_LIBMACH_SRC" -nt "$TEST_LIBMACH_BIN" ] || \
   [ "$LIBMACH_DIR/mach_traps.c" -nt "$TEST_LIBMACH_BIN" ]; then
    cc -O2 -I"$LIBMACH_DIR/include" \
        "$LIBMACH_DIR/mach_traps.c" "$TEST_LIBMACH_SRC" \
        -o "$TEST_LIBMACH_BIN" || {
        echo "FAIL build test_libmach"
        FAIL=$((FAIL+1))
    }
fi

if [ ! -x "$TEST_MACH_MSG_BIN" ] || \
   [ "$TEST_MACH_MSG_SRC" -nt "$TEST_MACH_MSG_BIN" ] || \
   [ "$LIBMACH_DIR/mach_traps.c" -nt "$TEST_MACH_MSG_BIN" ]; then
    cc -O2 -I"$LIBMACH_DIR/include" \
        "$LIBMACH_DIR/mach_traps.c" "$TEST_MACH_MSG_SRC" \
        -o "$TEST_MACH_MSG_BIN" || {
        echo "FAIL build test_mach_msg"
        FAIL=$((FAIL+1))
    }
fi

invoke_syscall() {
    name=$1
    NUM=$(sysctl -n "mach.syscall.$name" 2>/dev/null || echo -1)
    if [ "$NUM" -gt 0 ] 2>/dev/null && [ -x "$TEST_BIN" ]; then
        OUTPUT=$(MACH_SYSCALL_NUM=$NUM "$TEST_BIN" 2>&1)
        if [ $? -eq 0 ]; then
            echo "PASS invoke_$name: $OUTPUT"
            PASS=$((PASS+1))
        else
            echo "FAIL invoke_$name: $OUTPUT"
            FAIL=$((FAIL+1))
        fi
    else
        echo "SKIP invoke_$name: NUM=$NUM or binary missing"
    fi
}

# Sysctls.
check sysctl_debug_enable_read       sysctl mach.debug_enable
check sysctl_test_port_default       sysctl mach.test_port_lifecycle
check sysctl_test_port_write_5000    sysctl -w mach.test_port_lifecycle=5000
check sysctl_test_port_reread        sysctl mach.test_port_lifecycle
check sysctl_test_port_write_100     sysctl -w mach.test_port_lifecycle=100
check sysctl_mach_reply_port_no      sysctl mach.syscall.mach_reply_port
check sysctl_task_self_trap_no       sysctl mach.syscall.task_self_trap
check sysctl_thread_self_trap_no     sysctl mach.syscall.thread_self_trap
check sysctl_host_self_trap_no       sysctl mach.syscall.host_self_trap
check sysctl_mach_msg_trap_no        sysctl mach.syscall.mach_msg_trap
check sysctl_stats_ports_in_use      sysctl mach.stats.ports_in_use
check sysctl_stats_kmsgs_in_use      sysctl mach.stats.kmsgs_in_use
check sysctl_test_in_kernel_mqueue   sysctl mach.test_in_kernel_mqueue

# Wired syscalls. Each is invoked from a process that may or may not have
# Mach state; the NULL-guard wrapper means a 0 return is a valid PASS
# (MACH_PORT_NULL), not a failure.
invoke_syscall mach_reply_port
invoke_syscall task_self_trap
invoke_syscall thread_self_trap
invoke_syscall host_self_trap

# libmach userland shim: same syscalls, called by name through libmach
# instead of by raw syscall number. Validates that:
#   - sysctlbyname() resolves the syscall numbers at runtime
#   - the cached static works
#   - each entry point invokes the right syscall
# A return of 0 (MACH_PORT_NULL) is the documented behavior for
# pre-mach.ko processes; the goal here is "no crash, output reasonable."
if [ -x "$TEST_LIBMACH_BIN" ]; then
    OUTPUT=$("$TEST_LIBMACH_BIN" 2>&1)
    if [ $? -eq 0 ] && echo "$OUTPUT" | grep -q "mach_reply_port"; then
        echo "PASS libmach_trap_family:"
        echo "$OUTPUT" | sed 's/^/    /'
        PASS=$((PASS+1))
    else
        echo "FAIL libmach_trap_family: $OUTPUT"
        FAIL=$((FAIL+1))
    fi
else
    echo "SKIP libmach_trap_family: binary missing"
fi

# mach_msg receive-with-timeout=0 against an empty port. Expected:
# the syscall is reachable AND returns MACH_RCV_TIMED_OUT.
if [ -x "$TEST_MACH_MSG_BIN" ]; then
    OUTPUT=$("$TEST_MACH_MSG_BIN" 2>&1)
    if [ $? -eq 0 ]; then
        echo "PASS mach_msg_recv_timeout:"
        echo "$OUTPUT" | sed 's/^/    /'
        PASS=$((PASS+1))
    else
        echo "FAIL mach_msg_recv_timeout: $OUTPUT"
        FAIL=$((FAIL+1))
    fi
else
    echo "SKIP mach_msg_recv_timeout: binary missing"
fi

echo
echo "=== SMOKE TOTAL: $PASS pass, $FAIL fail ==="
[ "$FAIL" = "0" ]

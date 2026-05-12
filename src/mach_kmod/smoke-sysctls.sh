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

# Build the test helper if not present. Source travels with the repo.
TEST_BIN=/tmp/test_mach_syscall0
TEST_SRC="$(dirname "$0")/tests/test_mach_syscall0.c"
if [ ! -x "$TEST_BIN" ] || [ "$TEST_SRC" -nt "$TEST_BIN" ]; then
    cc -O2 -o "$TEST_BIN" "$TEST_SRC" || {
        echo "FAIL build test_mach_syscall0"
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

echo
echo "=== SMOKE TOTAL: $PASS pass, $FAIL fail ==="
[ "$FAIL" = "0" ]

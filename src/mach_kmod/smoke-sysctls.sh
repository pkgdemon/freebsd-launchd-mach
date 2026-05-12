#!/bin/sh
# Phase B regression smoke: hit every mach.* sysctl + the wired syscall.
# Usage: ssh root@bsd01 'bash -s' < smoke-sysctls.sh
# Output: per-test PASS/FAIL line; non-zero exit if anything fails.
set -u
PASS=0; FAIL=0
check() {
    name=$1; shift
    if "$@" >/tmp/smk.out 2>&1; then
        echo "PASS $name: $(cat /tmp/smk.out | head -1)"
        PASS=$((PASS+1))
    else
        echo "FAIL $name: $(cat /tmp/smk.out | head -1)"
        FAIL=$((FAIL+1))
    fi
}
# 1: debug_enable read
check sysctl1_debug_enable_read     sysctl mach.debug_enable
# 2: test_port_lifecycle default read
check sysctl2_test_port_default     sysctl mach.test_port_lifecycle
# 3: test_port_lifecycle write 5000
check sysctl2_test_port_write_5000  sysctl -w mach.test_port_lifecycle=5000
# 4: test_port_lifecycle read after write
check sysctl2_test_port_reread      sysctl mach.test_port_lifecycle
# 5: test_port_lifecycle write back to 100
check sysctl2_test_port_write_100   sysctl -w mach.test_port_lifecycle=100
# 6: mach.syscall.mach_reply_port read
check sysctl3_syscall_no_read       sysctl mach.syscall.mach_reply_port
# 7: actually invoke syscall #N (with NULL-guard wrapper, should return 0)
NUM=$(sysctl -n mach.syscall.mach_reply_port 2>/dev/null || echo -1)
if [ "$NUM" -gt 0 ] 2>/dev/null && [ -x /root/test_mach_reply ]; then
    OUTPUT=$(MACH_REPLY_PORT_NUM=$NUM /root/test_mach_reply 2>&1)
    if [ $? -eq 0 ]; then
        echo "PASS syscall4_invoke: $OUTPUT"
        PASS=$((PASS+1))
    else
        echo "FAIL syscall4_invoke: $OUTPUT"
        FAIL=$((FAIL+1))
    fi
else
    echo "SKIP syscall4_invoke: NUM=$NUM or binary missing"
fi
echo
echo "=== SMOKE TOTAL: $PASS pass, $FAIL fail ==="
[ "$FAIL" = "0" ]

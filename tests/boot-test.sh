#!/bin/sh
# boot-test.sh — boot the live ISO in qemu and confirm the system reaches
# multi-user mode by watching for /etc/rc's "Starting local daemons:"
# marker on the serial console. That single marker proves the entire
# pipeline worked: loader → kernel → cd9660 root mount → init →
# init.sh's gunion + init_chroot pivot → multi-user boot in the chroot.

set -eu

ISO=${1:?usage: boot-test.sh path/to/livecd.iso}

if [ ! -f "$ISO" ]; then
    echo "ERROR: $ISO not found"
    exit 1
fi

mkdir -p tests
LOG=tests/boot.log
EXP=tests/boot.exp

echo "==> boot test: $ISO"
ls -lh "$ISO"

# Pick acceleration. KVM if available; TCG fallback.
if [ -e /dev/kvm ]; then
    sudo chmod 666 /dev/kvm 2>/dev/null || true
fi
if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    ACCEL_FLAGS="-accel kvm -cpu host"
    echo "==> using KVM acceleration"
else
    ACCEL_FLAGS="-accel tcg,thread=single -cpu qemu64"
    echo "==> using TCG (single-thread)"
fi

# Find OVMF firmware
OVMF=""
for f in /usr/share/OVMF/OVMF_CODE.fd \
         /usr/share/ovmf/OVMF.fd \
         /usr/share/qemu/OVMF.fd; do
    if [ -f "$f" ]; then
        OVMF="$f"
        break
    fi
done
if [ -z "$OVMF" ]; then
    echo "ERROR: no OVMF firmware found"
    exit 1
fi
echo "==> using UEFI firmware: $OVMF"

export ACCEL_FLAGS OVMF

cat > "$EXP" <<'EOF'
set timeout 480
log_file -a tests/boot.log
log_user 1

set iso [lindex $argv 0]
set accel_flags [split $env(ACCEL_FLAGS) " "]

eval spawn qemu-system-x86_64 \
    -m 4G \
    -machine q35 \
    -bios $env(OVMF) \
    $accel_flags \
    -cdrom $iso -boot d \
    -display none -serial stdio \
    -no-reboot

# Stage 0: at the loader's autoboot countdown, drop into the OK prompt
# and enable the serial console for the kernel. /boot/loader.conf no
# longer sets console/boot_serial/etc. (clean console on real laptops
# and VBox where the floating comconsole UART line was producing junk
# chars at the loader prompt). Under qemu+OVMF -display none the
# loader's default `efi` console routes to serial so we can interact
# from here.
expect {
    timeout {
        puts "\nFAIL: didn't see loader autoboot prompt within 60s"
        exit 1
    }
    -re "Hit \\\[Enter\\\]" { send " " }
    "Booting"                { send " " }
    "FreeBSD/amd64 EFI"      { send " " }
}

expect {
    timeout {
        puts "\nFAIL: didn't reach loader OK prompt within 30s"
        exit 1
    }
    "OK " { puts "\n==> at loader prompt; setting serial console vars" }
}

send "set console=\"comconsole vidconsole\"\r"
expect "OK "
send "set boot_serial=YES\r"
expect "OK "
send "set comconsole_speed=115200\r"
expect "OK "
send "set boot_multicons=YES\r"
expect "OK "
send "boot\r"

# Stage 1: wait for the Phase B mach.ko smoke marker emitted by
# /etc/rc.local. By the time this fires, the boot pipeline has worked:
# loader preloaded mach.ko -> kernel up -> /init.sh as PID 1 -> unionfs
# pivot -> stock init in chroot -> rc multi-user sequence -> rc.local
# ran kldstat -m mach. The marker proves mach.ko is loaded.
expect {
    timeout {
        puts "\nFAIL: MACH-SMOKE-OK marker not seen within 8 minutes"
        exit 1
    }
    "MACH-SMOKE-FAIL" {
        puts "\nFAIL: mach.ko did NOT load on boot"
        exit 1
    }
    "MACH-SMOKE-OK" { puts "\nOK: mach.ko is loaded" }
}

# Stage 2: wait for the getty "login:" prompt. Boot is complete.
expect {
    timeout {
        puts "\nFAIL: 'login:' prompt not seen within 8 minutes"
        exit 1
    }
    "login:" { puts "\nOK: boot reached the login prompt" }
}

# Stage 3: send "root" and confirm login(1) actually responds. Either
# "Password:" (login asked, so login binary is alive — proves the post-
# getty userland is intact) or a shell prompt (passwordless root login
# succeeded — even better) is a pass. Silence is failure.
send "root\r"
expect {
    timeout {
        puts "\nFAIL: login(1) did not respond after sending 'root'"
        exit 1
    }
    "Password:" { puts "OK: login(1) accepted username, requesting password" }
    -re {[#%$] $} { puts "OK: passwordless root login succeeded; got shell prompt" }
}

close
wait
exit 0
EOF

expect "$EXP" "$ISO"
echo "==> boot-test PASSED"

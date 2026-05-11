#!/bin/sh
# build.sh — assemble a FreeBSD live ISO using the init_chroot architecture
# with an in-kernel unionfs + tmpfs writable overlay:
#   cd9660 = kernel root; vnode-mounted rootfs.uzip is the read-only lower;
#   tmpfs is the writable upper; in-kernel unionfs combines them; pivot
#   via init_chroot kenv (no preload, no mfsroot, no reboot -r).
# Runs on FreeBSD (host or vmactions VM). Produces out/livecd.iso.
#
# Phase A target: vanilla FreeBSD ISO that boots stock /sbin/init as PID 1.
# No Mach kernel module yet, no launchd, no configd. Phase B will add
# mach.ko build + install steps.

set -eu

: "${FREEBSD_VERSION:=15.0}"
: "${COMPRESS:=zstd}"
: "${LABEL:=LIVECD}"
ARCH=${ARCH:-amd64}

# Note: no LIVE_HEADROOM in this variant. The lower UFS is sized exactly
# to content; writable headroom comes from the tmpfs upper at boot, which
# is page-allocated on demand and bounded by host RAM + swap rather than
# by a build-time constant.

ROOT=$(cd "$(dirname "$0")" && pwd)
WORK=$ROOT/work
OUT=$ROOT/out
DIST=$ROOT/distfiles

MIRROR="https://download.freebsd.org/ftp/releases/${ARCH}/${FREEBSD_VERSION}-RELEASE"

mkdir -p "$WORK" "$OUT" "$DIST"

# Clean any prior partial build (but keep distfiles cached)
rm -rf "$WORK"/* "$OUT"/*

echo "==> build: FreeBSD $FREEBSD_VERSION ($ARCH), compress=$COMPRESS"

#
# 1. fetch base.txz + kernel.txz + src.txz
#
# src.txz gives us /usr/src/release/amd64/mkisoimages.sh — the FreeBSD
# release engineering script that builds the hybrid (BIOS + UEFI El
# Torito + GPT-overlaid for USB-stick dd) cd9660. Replaces the
# hand-rolled makefs invocation we used to do; the script's hybrid-GPT
# step at the end of mkisoimages.sh is what makes the same .iso file
# bootable from both optical media and a dd'd USB stick.
#
for f in base.txz kernel.txz src.txz; do
    if [ ! -f "$DIST/$f" ]; then
        echo "==> downloading $f"
        fetch -o "$DIST/$f" "$MIRROR/$f"
    fi
done

#
# 2. extract into rootfs staging dir
#
echo "==> extracting base+kernel"
mkdir -p "$WORK/rootfs"
tar -xJf "$DIST/base.txz"   -C "$WORK/rootfs"
tar -xJf "$DIST/kernel.txz" -C "$WORK/rootfs"

# base.txz ships /etc/login.conf but not the compiled /etc/login.conf.db.
# Without the .db, login_getclass() can't find any class and logs a noisy
# warning at boot ("login_getclass: unknown class 'daemon'"). The FreeBSD
# installer rebuilds it via cap_mkdb during install; we have to do the
# same since we skip bsdinstall.
cap_mkdb "$WORK/rootfs/etc/login.conf"

# Same idea for the password databases. base.txz may or may not ship the
# *.db files depending on version; rebuild them to be safe so getpwnam()
# and friends work without warnings.
pwd_mkdb -p -d "$WORK/rootfs/etc" "$WORK/rootfs/etc/master.passwd"

#
# 3. chroot: install runtime pkgs (pkglist.txt) only.
#    Phase A has no buildpkgs (empty buildpkgs.txt), no system-domain
#    build, no launchd/configd build, no kmodloader install. Just the
#    runtime packages needed to boot a usable FreeBSD live ISO.
#
RUNTIME_PKGS=$(grep -v '^[[:space:]]*#' "$ROOT/pkglist.txt" 2>/dev/null | grep -v '^[[:space:]]*$' || true)

if [ -n "$RUNTIME_PKGS" ]; then
    cp /etc/resolv.conf "$WORK/rootfs/etc/resolv.conf"
    mount -t devfs devfs "$WORK/rootfs/dev"
    cleanup_chroot() {
        umount -f "$WORK/rootfs/dev" 2>/dev/null || true
        rm -f "$WORK/rootfs/etc/resolv.conf"
    }
    trap cleanup_chroot EXIT INT TERM

    chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes IGNORE_OSVERSION=yes pkg bootstrap -f

    echo "==> installing runtime packages:"
    echo "$RUNTIME_PKGS" | sed 's/^/    /'
    # shellcheck disable=SC2086
    # LICENSES_ACCEPTED=NVIDIA: nvidia-drm-latest-kmod (if it appears in
    # pkglist.txt later) is a restricted-distribution blob and pkg refuses
    # to install it without explicit license acceptance.
    chroot "$WORK/rootfs" env \
        ASSUME_ALWAYS_YES=yes \
        IGNORE_OSVERSION=yes \
        LICENSES_ACCEPTED=NVIDIA \
        pkg install -y $RUNTIME_PKGS

    # ---- dhcpcd: silence DHCPv6 retry spam ----
    # The dhcpcd port ships /usr/local/etc/dhcpcd.conf as @sample; pkg
    # copies it to dhcpcd.conf at install. Append nodhcp6 so dhcpcd
    # doesn't attempt DHCPv6 on networks where the router advertises
    # the M-flag but the DHCPv6 server returns "No Addresses Available".
    if [ -f "$WORK/rootfs/usr/local/etc/dhcpcd.conf" ]; then
        cat >> "$WORK/rootfs/usr/local/etc/dhcpcd.conf" <<'EOF'

# Live ISO overrides (see build.sh comment).
nodhcp6
quiet
EOF
    fi

    cleanup_chroot
    trap - EXIT INT TERM
fi

#
# 4. trim rootfs of things not needed at runtime
#
echo "==> slimming rootfs"
rm -rf \
    "$WORK/rootfs/usr/share/man" \
    "$WORK/rootfs/usr/share/doc" \
    "$WORK/rootfs/usr/share/info" \
    "$WORK/rootfs/usr/share/locale" \
    "$WORK/rootfs/usr/share/games" \
    "$WORK/rootfs/usr/share/examples" \
    "$WORK/rootfs/usr/share/openssl" \
    "$WORK/rootfs/usr/share/dict" \
    "$WORK/rootfs/usr/share/calendar" \
    "$WORK/rootfs/usr/include" \
    "$WORK/rootfs/usr/tests" \
    "$WORK/rootfs/usr/lib/debug" \
    "$WORK/rootfs/usr/libdata/lint" \
    "$WORK/rootfs/var/db/etcupdate"
find "$WORK/rootfs/boot/kernel" -name '*.symbols' -delete 2>/dev/null || true

#
# 5. apply local overlays (etc/rc.conf, etc/motd.template, ...)
#
if [ -d "$ROOT/overlays" ]; then
    echo "==> applying overlays"
    cp -aR "$ROOT/overlays/." "$WORK/rootfs/"
fi

# rc.local needs to be executable
[ -f "$WORK/rootfs/etc/rc.local" ] && chmod +x "$WORK/rootfs/etc/rc.local"

#
# 6. minimal /etc/fstab; root mounted by unionfs at boot, no entries needed
#
cat > "$WORK/rootfs/etc/fstab" <<'EOF'
# Live system: root is the unionfs merged view (read-only UFS lower +
# tmpfs upper, layered in the ramdisk-style init phase and then exposed
# as / via init_chroot).
EOF

#
# 7. makefs UFS without an explicit -s. The writable upper is tmpfs at
#    boot, so the lower UFS doesn't need user-visible headroom -- only
#    enough room for UFS internal overhead.
#
CONTENT_BYTES=$(du -sk "$WORK/rootfs" | awk '{print $1*1024}')
echo "==> rootfs content = $CONTENT_BYTES bytes ($((CONTENT_BYTES / 1024 / 1024)) MiB)"

echo "==> makefs ffs (auto-sized)"
makefs -t ffs -o version=2,label=ROOTFS \
    "$WORK/rootfs.ufs" "$WORK/rootfs"
ls -lh "$WORK/rootfs.ufs"

mkdir -p "$WORK/cdroot"
case "$COMPRESS" in
    zstd) MKUZIP_FLAGS="-A zstd -C 19 -d -s 262144" ;;
    zlib) MKUZIP_FLAGS="-d -s 65536" ;;
    *)    echo "ERROR: unknown COMPRESS=$COMPRESS"; exit 1 ;;
esac
echo "==> mkuzip $MKUZIP_FLAGS"
mkuzip $MKUZIP_FLAGS -j "$(sysctl -n hw.ncpu)" \
    -o "$WORK/cdroot/rootfs.uzip" "$WORK/rootfs.ufs"

# Stage the init environment on the cd9660 root.
echo "==> staging init environment on cd9660"
mkdir -p "$WORK/cdroot/sbin" "$WORK/cdroot/rescue" "$WORK/cdroot/sysroot" \
         "$WORK/cdroot/upper" "$WORK/cdroot/dev" "$WORK/cdroot/etc"

# /rescue: statically-linked busybox-equivalent. Use a tar pipe to
# preserve hardlinks (FreeBSD's `cp -a` does NOT preserve them).
( cd "$WORK/rootfs" && tar cf - rescue ) | ( cd "$WORK/cdroot" && tar xf - )

# Ship /etc/login.conf (+ compiled .db) on the cd9660 root.
for f in passwd master.passwd group pwd.db spwd.db; do
    if [ -f "$WORK/rootfs/etc/$f" ]; then
        cp "$WORK/rootfs/etc/$f" "$WORK/cdroot/etc/$f"
    fi
done

cp "$WORK/rootfs/etc/login.conf" "$WORK/cdroot/etc/login.conf"
[ -f "$WORK/rootfs/etc/login.conf.db" ] && \
    cp "$WORK/rootfs/etc/login.conf.db" "$WORK/cdroot/etc/login.conf.db"

# pivot script
cp "$ROOT/ramdisk/init.sh" "$WORK/cdroot/init.sh"
chmod +x "$WORK/cdroot/init.sh"

ls -lh "$WORK/cdroot/rootfs.uzip"

#
# 8. stage /boot on the cd9660 carrier — but ONLY the loader-needed bits.
#
echo "==> staging minimal /boot on cd9660"
mkdir -p "$WORK/cdroot/boot/kernel"

# Bootloader pieces (whichever exist; vary by FreeBSD release/arch)
for f in cdboot loader loader.efi loader_lua loader_lua.efi \
         loader_simp loader_simp.efi pmbr isoboot boot1.efi \
         gptboot defaults device.hints lua fonts; do
    if [ -e "$WORK/rootfs/boot/$f" ]; then
        cp -aR "$WORK/rootfs/boot/$f" "$WORK/cdroot/boot/"
    fi
done

# Kernel binary, gzipped.
gzip -9c "$WORK/rootfs/boot/kernel/kernel" > "$WORK/cdroot/boot/kernel/kernel.gz"
ls -lh "$WORK/cdroot/boot/kernel/kernel.gz" \
       "$WORK/rootfs/boot/kernel/kernel"

# Modules: only what we need at boot. Phase B will add mach.ko here.
BOOT_MODULES="geom_uzip.ko unionfs.ko \
              acpi.ko \
              virtio.ko virtio_pci.ko virtio_blk.ko virtio_scsi.ko \
              ahci.ko mfi.ko"
for m in $BOOT_MODULES; do
    if [ -f "$WORK/rootfs/boot/kernel/$m" ]; then
        cp "$WORK/rootfs/boot/kernel/$m" "$WORK/cdroot/boot/kernel/"
    fi
done

cp "$ROOT/boot/loader.conf" "$WORK/cdroot/boot/loader.conf"

# /boot/firmware symlink — see freebsd-launchd build.sh for rationale.
ln -sf /sysroot/boot/firmware "$WORK/cdroot/boot/firmware"

echo "==> /boot on cd9660:"
du -sh "$WORK/cdroot/boot" "$WORK/cdroot/boot/kernel" || true
ls -la "$WORK/cdroot/boot/kernel/" || true
ls -la "$WORK/cdroot/boot/firmware" || true

#
# 9. extract src.txz to expose FreeBSD's release scripts.
#
echo "==> extracting src.txz for FreeBSD release scripts"
mkdir -p "$WORK/freebsd-src"
tar -xJf "$DIST/src.txz" -C "$WORK/freebsd-src"

#
# 10. stage cd9660-direct UEFI fallback.
#
mkdir -p "$WORK/cdroot/EFI/BOOT"
if [ -f "$WORK/rootfs/boot/loader_lua.efi" ]; then
    cp "$WORK/rootfs/boot/loader_lua.efi" "$WORK/cdroot/EFI/BOOT/BOOTX64.EFI"
elif [ -f "$WORK/rootfs/boot/loader.efi" ]; then
    cp "$WORK/rootfs/boot/loader.efi" "$WORK/cdroot/EFI/BOOT/BOOTX64.EFI"
else
    echo "ERROR: no loader.efi found in base.txz boot/"
    exit 1
fi

#
# 11. build hybrid ISO via FreeBSD's own mkisoimages.sh.
#
echo "==> building hybrid ISO via mkisoimages.sh"
MKISO="$WORK/freebsd-src/usr/src/release/amd64/mkisoimages.sh"
[ -f "$MKISO" ] || { echo "ERROR: $MKISO not found in src.txz" >&2; exit 1; }
chmod +x "$MKISO"
"$MKISO" -b "$LABEL" "$OUT/livecd.iso" "$WORK/cdroot"

ls -lh "$OUT/livecd.iso"
sha256 "$OUT/livecd.iso" 2>/dev/null || sha256sum "$OUT/livecd.iso"

echo
echo "==> cdroot size breakdown:"
du -sh "$WORK/cdroot"/* 2>/dev/null | sort -h
echo
echo "==> ISO total: $(ls -lh "$OUT/livecd.iso" | awk '{print $5}')"
echo "==> DONE"

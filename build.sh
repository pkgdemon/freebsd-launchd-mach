#!/bin/sh
# build.sh — assemble a FreeBSD live ISO using the init_chroot architecture
# with an in-kernel unionfs + tmpfs writable overlay:
#   cd9660 = kernel root; vnode-mounted rootfs.uzip is the read-only lower;
#   tmpfs is the writable upper; in-kernel unionfs combines them; pivot
#   via init_chroot kenv (no preload, no mfsroot, no reboot -r).
# Runs on FreeBSD (host or vmactions VM). Produces out/livecd.iso.
#
# Phase A target: FreeBSD live ISO with stock /sbin/init as PID 1 AND the
# gershwin/GNUstep system-domain libraries (libdispatch, libobjc2,
# libs-base, libs-corebase) pre-installed under /System/Library/. No Mach
# kernel module yet, no launchd, no configd. Phase B adds mach.ko.

set -eu

: "${FREEBSD_VERSION:=15.0}"
: "${COMPRESS:=zstd}"
: "${LABEL:=LIVECD}"
ARCH=${ARCH:-amd64}

ROOT=$(cd "$(dirname "$0")" && pwd)
WORK=$ROOT/work
OUT=$ROOT/out
DIST=$ROOT/distfiles
REPOS=$ROOT/repos

MIRROR="https://download.freebsd.org/ftp/releases/${ARCH}/${FREEBSD_VERSION}-RELEASE"

mkdir -p "$WORK" "$OUT" "$DIST" "$REPOS"

# Clean any prior partial build (but keep distfiles + repos cached)
rm -rf "$WORK"/* "$OUT"/*

echo "==> build: FreeBSD $FREEBSD_VERSION ($ARCH), compress=$COMPRESS"

#
# 0. host-side: clone or update the gershwin system-domain upstreams into
#    repos/. Order is significant: tools-make writes the GNUstep.conf that
#    libobjc2 reads, libobjc2 needs BlocksRuntime from libdispatch,
#    libs-base/libs-corebase install through gnustep-make. Tracking
#    upstream HEAD; no pinning. Chroot stays git-free — these clones live
#    on the host and get rsynced in below.
#
echo "==> cloning/updating gershwin system-domain upstreams"
UPSTREAMS="
https://github.com/apple/swift-corelibs-libdispatch.git
https://github.com/gnustep/tools-make.git
https://github.com/gnustep/libobjc2.git
https://github.com/Tessil/robin-map.git
https://github.com/gnustep/libs-base.git
https://github.com/gnustep/libs-corebase.git
"
for repo in $UPSTREAMS; do
    name=$(basename "$repo" .git)
    if [ -d "$REPOS/$name/.git" ]; then
        echo "    updating $name"
        ( cd "$REPOS/$name" && git fetch --all --tags && git pull --ff-only )
    else
        echo "    cloning $name"
        git clone "$repo" "$REPOS/$name"
    fi
done

#
# 1. fetch base.txz + kernel.txz + src.txz
#
# src.txz gives us /usr/src/release/amd64/mkisoimages.sh — the FreeBSD
# release engineering script that builds the hybrid (BIOS + UEFI El
# Torito + GPT-overlaid for USB-stick dd) cd9660.
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

# Rebuild login + passwd .db files (bsdinstall would normally do this).
cap_mkdb "$WORK/rootfs/etc/login.conf"
pwd_mkdb -p -d "$WORK/rootfs/etc" "$WORK/rootfs/etc/master.passwd"

#
# 3. chroot: runtime pkgs (pkglist.txt) + build pkgs (buildpkgs.txt) +
#    system-domain build (libdispatch + GNUstep stack) + buildpkgs purge.
#    Single chroot session for all of it; build pkgs go in and out before
#    the slim pass so they don't ship in the ISO.
#
RUNTIME_PKGS=$(grep -v '^[[:space:]]*#' "$ROOT/pkglist.txt"   2>/dev/null | grep -v '^[[:space:]]*$' || true)
BUILD_PKGS=$(  grep -v '^[[:space:]]*#' "$ROOT/buildpkgs.txt" 2>/dev/null | grep -v '^[[:space:]]*$' || true)

if [ -n "$RUNTIME_PKGS" ] || [ -n "$BUILD_PKGS" ]; then
    cp /etc/resolv.conf "$WORK/rootfs/etc/resolv.conf"
    mount -t devfs devfs "$WORK/rootfs/dev"
    cleanup_chroot() {
        umount -f "$WORK/rootfs/dev" 2>/dev/null || true
        rm -f "$WORK/rootfs/etc/resolv.conf"
        rm -rf "$WORK/rootfs/tmp/repos"
    }
    trap cleanup_chroot EXIT INT TERM

    chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes IGNORE_OSVERSION=yes pkg bootstrap -f

    if [ -n "$RUNTIME_PKGS" ]; then
        echo "==> installing runtime packages:"
        echo "$RUNTIME_PKGS" | sed 's/^/    /'
        # shellcheck disable=SC2086
        chroot "$WORK/rootfs" env \
            ASSUME_ALWAYS_YES=yes \
            IGNORE_OSVERSION=yes \
            LICENSES_ACCEPTED=NVIDIA \
            pkg install -y $RUNTIME_PKGS

        # ---- dhcpcd: silence DHCPv6 retry spam ----
        if [ -f "$WORK/rootfs/usr/local/etc/dhcpcd.conf" ]; then
            cat >> "$WORK/rootfs/usr/local/etc/dhcpcd.conf" <<'EOF'

# Live ISO overrides (see build.sh comment).
nodhcp6
quiet
EOF
        fi
    fi

    if [ -n "$BUILD_PKGS" ]; then
        echo "==> installing build packages:"
        echo "$BUILD_PKGS" | sed 's/^/    /'
        # shellcheck disable=SC2086
        chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes IGNORE_OSVERSION=yes \
            pkg install -y $BUILD_PKGS

        # Host-cloned upstreams into chroot; chroot stays git-free.
        echo "==> rsyncing repos/ -> chroot:/tmp/repos/"
        mkdir -p "$WORK/rootfs/tmp/repos"
        rsync -a --delete "$REPOS/" "$WORK/rootfs/tmp/repos/"

        # libobjc2's CMakeLists FetchContent_Declare(robinmap) calls git
        # at configure-time — would fail in our git-free chroot. Rewrite
        # the Declare to point SOURCE_DIR at the sibling robin-map clone
        # we just rsynced.
        echo "==> patching libobjc2 FetchContent(robinmap) -> SOURCE_DIR"
        sed -i '' \
            -e 's|GIT_REPOSITORY https://github.com/Tessil/robin-map/|SOURCE_DIR /tmp/repos/robin-map)|' \
            -e '/GIT_TAG[[:space:]]*v1\.4\.0)/d' \
            "$WORK/rootfs/tmp/repos/libobjc2/CMakeLists.txt"

        echo "==> building gershwin system-domain libraries in chroot"
        chroot "$WORK/rootfs" /bin/sh -ex <<'CHROOT_BUILD'
MAKE_CMD=gmake
CPUS=$(sysctl -n hw.ncpu)
REPOS_DIR=/tmp/repos

# libdispatch
mkdir -p "$REPOS_DIR/swift-corelibs-libdispatch/Build"
cd "$REPOS_DIR/swift-corelibs-libdispatch/Build"
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/System/Library \
  -DCMAKE_INSTALL_LIBDIR=Libraries \
  -DINSTALL_DISPATCH_HEADERS_DIR=/System/Library/Headers/dispatch \
  -DINSTALL_BLOCK_HEADERS_DIR=/System/Library/Headers \
  -DINSTALL_OS_HEADERS_DIR=/System/Library/Headers/os \
  -DINSTALL_PRIVATE_HEADERS=ON \
  -DCMAKE_INSTALL_MANDIR=Documentation/man \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
"$MAKE_CMD" -j"$CPUS"
"$MAKE_CMD" install

# tools-make
cd "$REPOS_DIR/tools-make"
$MAKE_CMD distclean 2>/dev/null || true
./configure \
  --with-config-file=/System/Library/Preferences/GNUstep.conf \
  --with-layout=gershwin \
  --with-library-combo=ng-gnu-gnu \
  --with-objc-lib-flag=" " \
  LDFLAGS="-L/System/Library/Libraries" \
  CPPFLAGS="-I/System/Library/Headers" \
  libobjc_LIBS=" "
$MAKE_CMD
$MAKE_CMD install

# Source the GNUstep environment now that tools-make has installed it.
. /System/Library/Makefiles/GNUstep.sh

# libobjc2
rm -rf "$REPOS_DIR/libobjc2/Build"
mkdir -p "$REPOS_DIR/libobjc2/Build"
cd "$REPOS_DIR/libobjc2/Build"
cmake .. \
  -DGNUSTEP_INSTALL_TYPE=SYSTEM \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DEMBEDDED_BLOCKS_RUNTIME=OFF \
  -DBlocksRuntime_INCLUDE_DIR=/System/Library/Headers \
  -DBlocksRuntime_LIBRARIES=/System/Library/Libraries/libBlocksRuntime.so
"$MAKE_CMD" -j"$CPUS"
"$MAKE_CMD" install

# libs-base (Foundation)
export GNUSTEP_INSTALLATION_DOMAIN="SYSTEM"
cd "$REPOS_DIR/libs-base"
# --disable-tls: launchd doesn't speak TLS, and pulling in libgnutls/openssl
# would bloat the ISO for NSStream/NSURLConnection functionality we don't use.
./configure \
  --with-dispatch-include=/System/Library/Headers \
  --with-dispatch-library=/System/Library/Libraries \
  --disable-tls
$MAKE_CMD -j"$CPUS"
$MAKE_CMD install
$MAKE_CMD clean

# libs-corebase (CoreFoundation)
cd "$REPOS_DIR/libs-corebase"
./configure \
  CPPFLAGS="-I/System/Library/Headers" \
  LDFLAGS="-L/System/Library/Libraries"
$MAKE_CMD -j"$CPUS"
$MAKE_CMD install
$MAKE_CMD clean
CHROOT_BUILD

        rm -rf "$WORK/rootfs/tmp/repos"

        # ---- ldconfig hint for /System/Library/Libraries ----
        # FreeBSD's /etc/rc.d/ldconfig at boot reads $ldconfig_local_dirs
        # (default /usr/local/libdata/ldconfig) and adds each listed
        # directory to the runtime linker hints. The `ldconfig -m` here
        # primes the hints DB at build time so the ISO boots with
        # /System/Library/Libraries/* immediately discoverable.
        echo "==> writing ldconfig hint for /System/Library/Libraries"
        mkdir -p "$WORK/rootfs/usr/local/libdata/ldconfig"
        echo "/System/Library/Libraries" \
            > "$WORK/rootfs/usr/local/libdata/ldconfig/freebsd-launchd-mach"
        chroot "$WORK/rootfs" ldconfig -m /usr/local/lib /System/Library/Libraries

        echo "==> purging build packages"
        # shellcheck disable=SC2086
        chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
            pkg delete -y $BUILD_PKGS
        chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
            pkg autoremove -y || true
    fi

    cleanup_chroot
    trap - EXIT INT TERM
fi

#
# 3a. extract src.txz to $WORK/freebsd-src. Used for two things in
#     subsequent steps:
#       - kernel sources for the mach.ko out-of-tree build (3b)
#       - FreeBSD's release scripts (mkisoimages.sh) at step 11
#     One extraction, two consumers.
#
echo "==> extracting src.txz for kernel sources + release scripts"
mkdir -p "$WORK/freebsd-src"
tar -xJf "$DIST/src.txz" -C "$WORK/freebsd-src"

#
# 3b. build mach.ko against the freshly-extracted kernel sources and
#     install it into $WORK/rootfs/boot/kernel/mach.ko so it ships
#     inside rootfs.uzip. Step 8 below also copies it onto the cd9660
#     so the loader can preload it before init runs.
#
echo "==> building mach.ko"
"$ROOT/make-mach-kmod.sh" \
    --sysdir="$WORK/freebsd-src/usr/src/sys" \
    --prefix="$WORK/rootfs"
ls -lh "$WORK/rootfs/boot/kernel/mach.ko"

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
#    boot, so the lower UFS doesn't need user-visible headroom.
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

echo "==> staging init environment on cd9660"
mkdir -p "$WORK/cdroot/sbin" "$WORK/cdroot/rescue" "$WORK/cdroot/sysroot" \
         "$WORK/cdroot/upper" "$WORK/cdroot/dev" "$WORK/cdroot/etc"

# /rescue: use a tar pipe to preserve hardlinks (cp -a does not).
( cd "$WORK/rootfs" && tar cf - rescue ) | ( cd "$WORK/cdroot" && tar xf - )

# Ship /etc/login.conf (+ compiled .db) and password files on cd9660.
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
# 8. stage /boot on the cd9660 carrier — only the loader-needed bits.
#
echo "==> staging minimal /boot on cd9660"
mkdir -p "$WORK/cdroot/boot/kernel"

for f in cdboot loader loader.efi loader_lua loader_lua.efi \
         loader_simp loader_simp.efi pmbr isoboot boot1.efi \
         gptboot defaults device.hints lua fonts; do
    if [ -e "$WORK/rootfs/boot/$f" ]; then
        cp -aR "$WORK/rootfs/boot/$f" "$WORK/cdroot/boot/"
    fi
done

# Kernel binary, gzipped (loader's gzipfs layer decompresses on read).
gzip -9c "$WORK/rootfs/boot/kernel/kernel" > "$WORK/cdroot/boot/kernel/kernel.gz"
ls -lh "$WORK/cdroot/boot/kernel/kernel.gz" \
       "$WORK/rootfs/boot/kernel/kernel"

# Modules to preload from cd9660 (before unionfs pivot).
# mach.ko is here so loader brings it in before init runs.
BOOT_MODULES="geom_uzip.ko unionfs.ko \
              acpi.ko \
              virtio.ko virtio_pci.ko virtio_blk.ko virtio_scsi.ko \
              ahci.ko mfi.ko \
              mach.ko"
for m in $BOOT_MODULES; do
    if [ -f "$WORK/rootfs/boot/kernel/$m" ]; then
        cp "$WORK/rootfs/boot/kernel/$m" "$WORK/cdroot/boot/kernel/"
    fi
done

cp "$ROOT/boot/loader.conf" "$WORK/cdroot/boot/loader.conf"

# /boot/firmware symlink — see comment in freebsd-launchd build.sh.
ln -sf /sysroot/boot/firmware "$WORK/cdroot/boot/firmware"

echo "==> /boot on cd9660:"
du -sh "$WORK/cdroot/boot" "$WORK/cdroot/boot/kernel" || true
ls -la "$WORK/cdroot/boot/kernel/" || true
ls -la "$WORK/cdroot/boot/firmware" || true

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

#
# 12. package mach.ko as a standalone release tarball. Users on a stock
#     FreeBSD install can fetch this, untar to /, kldload mach, without
#     building anything.
#
echo "==> packaging mach.ko standalone tarball"
MACHKO_PKG_DIR="$WORK/mach-kmod-pkg"
MACHKO_BASENAME="mach.ko-FreeBSD-${FREEBSD_VERSION}-${ARCH}"
mkdir -p "$MACHKO_PKG_DIR/boot/kernel"
cp "$WORK/rootfs/boot/kernel/mach.ko" "$MACHKO_PKG_DIR/boot/kernel/mach.ko"
cat > "$MACHKO_PKG_DIR/README" <<EOF
mach.ko — out-of-tree FreeBSD Mach IPC kernel module.

Built against: FreeBSD ${FREEBSD_VERSION} (${ARCH})
Built on:      $(date -u +%Y-%m-%dT%H:%M:%SZ)

Install:
  tar -xJf ${MACHKO_BASENAME}.tar.gz -C /
  echo 'mach_load="YES"' >> /boot/loader.conf
  # reboot, or kldload mach
EOF
( cd "$MACHKO_PKG_DIR" && tar -czf "$OUT/${MACHKO_BASENAME}.tar.gz" boot README )
ls -lh "$OUT/${MACHKO_BASENAME}.tar.gz"
sha256 "$OUT/${MACHKO_BASENAME}.tar.gz" 2>/dev/null || sha256sum "$OUT/${MACHKO_BASENAME}.tar.gz"

echo
echo "==> cdroot size breakdown:"
du -sh "$WORK/cdroot"/* 2>/dev/null | sort -h
echo
echo "==> ISO total: $(ls -lh "$OUT/livecd.iso" | awk '{print $5}')"
echo "==> mach.ko tarball: $(ls -lh "$OUT/${MACHKO_BASENAME}.tar.gz" | awk '{print $5}')"
echo "==> DONE"

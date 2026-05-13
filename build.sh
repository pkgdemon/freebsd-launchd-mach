#!/bin/sh
# build.sh — assemble a FreeBSD live ISO using the init_chroot architecture
# with an in-kernel unionfs + tmpfs writable overlay:
#   cd9660 = kernel root; vnode-mounted rootfs.uzip is the read-only lower;
#   tmpfs is the writable upper; in-kernel unionfs combines them; pivot
#   via init_chroot kenv (no preload, no mfsroot, no reboot -r).
# Runs on FreeBSD (host or vmactions VM). Produces out/livecd.iso.
#
# Current scope: FreeBSD live ISO with stock /sbin/init as PID 1 and
# mach.ko preloaded by the loader. Base comes from pkgbase
# (pkg.freebsd.org/FreeBSD:<major>:<arch>/base_latest), curated via
# pkglist-base.txt — no full base.txz/kernel.txz extraction. Userland
# gershwin/GNUstep stack is not built here; to be added back via a
# separate mechanism.

set -eu

: "${FREEBSD_VERSION:=15.0}"
: "${COMPRESS:=zstd}"
: "${LABEL:=LIVECD}"
ARCH=${ARCH:-amd64}

# pkgbase ABI: pkg.freebsd.org organizes repos under FreeBSD:<major>:<arch>.
# Strip any minor version (15.0 -> 15) so the URL resolves.
PKG_MAJOR=${FREEBSD_VERSION%%.*}
PKG_ABI="FreeBSD:${PKG_MAJOR}:${ARCH}"

ROOT=$(cd "$(dirname "$0")" && pwd)
WORK=$ROOT/work
OUT=$ROOT/out
DIST=$ROOT/distfiles
PKG_CONFIG=$WORK/pkg-config

MIRROR="https://download.freebsd.org/ftp/releases/${ARCH}/${FREEBSD_VERSION}-RELEASE"

mkdir -p "$WORK" "$OUT" "$DIST"

# Clean any prior partial build (but keep distfiles cached)
rm -rf "$WORK"/* "$OUT"/*

echo "==> build: FreeBSD $FREEBSD_VERSION ($ARCH), compress=$COMPRESS"

#
# 1. fetch src.txz.
#
# Base/kernel come from pkgbase (pkg.freebsd.org/${PKG_ABI}/base_latest)
# at step 2 — no more base.txz / kernel.txz.
#
# src.txz is still fetched on the host for two consumers:
#   - kernel sources for the mach.ko out-of-tree build (3b)
#   - mkisoimages.sh release script at step 11
# Neither lives inside the chroot.
#
if [ ! -f "$DIST/src.txz" ]; then
    echo "==> downloading src.txz"
    fetch -o "$DIST/src.txz" "$MIRROR/src.txz"
fi

#
# 2. install curated pkgbase set into rootfs staging dir.
#
# Replaces the wholesale `tar -xJf base.txz + kernel.txz` with a
# host-driven `pkg -R config -r rootfs install -y ...` against
# pkg.freebsd.org/${PKG_ABI}/base_latest. The package list is hand
# curated (pkglist-base.txt for runtime; buildpkgs-base.txt for build-
# only FreeBSD-* like clang/lld/-dev, purged before mkuzip alongside
# buildpkgs.txt so build tooling never ships in the ISO).
#
# ABI/OSVERSION/IGNORE_OSVERSION env let us install a ${PKG_ABI} target
# from a host pkg of any version. Same trick gershwin-on-freebsd uses.
#
# Note: cap_mkdb / pwd_mkdb are no longer needed here — the FreeBSD-rc
# / FreeBSD-runtime pkg post-install scripts rebuild login.conf.db and
# pwd.db / spwd.db automatically.
#
echo "==> writing pkgbase repo config (${PKG_ABI}, base_latest)"
mkdir -p "$PKG_CONFIG" "$WORK/rootfs"
cat > "$PKG_CONFIG/FreeBSD-base.conf" <<EOF
FreeBSD-base: {
  url: "https://pkg.freebsd.org/${PKG_ABI}/base_latest",
  enabled: yes
}
EOF

BASE_PKGS=$(     grep -v '^[[:space:]]*#' "$ROOT/pkglist-base.txt"   2>/dev/null | grep -v '^[[:space:]]*$' || true)
BASE_BUILD_PKGS=$(grep -v '^[[:space:]]*#' "$ROOT/buildpkgs-base.txt" 2>/dev/null | grep -v '^[[:space:]]*$' || true)

if [ -z "$BASE_PKGS" ]; then
    echo "ERROR: pkglist-base.txt is empty; refusing to build empty rootfs" >&2
    exit 1
fi

# Single combined install — runtime + build-only — so the dep solver
# resolves once. buildpkgs-base.txt entries get pkg deleted at the end of
# step 3 alongside buildpkgs.txt.
echo "==> installing pkgbase runtime + build pkgs into $WORK/rootfs"
echo "$BASE_PKGS" | sed 's/^/    runtime  /'
echo "$BASE_BUILD_PKGS" | sed 's/^/    build    /'
# shellcheck disable=SC2086
env ABI="${PKG_ABI}" \
    OSVERSION="${PKG_MAJOR}00000" \
    IGNORE_OSVERSION=yes \
    ASSUME_ALWAYS_YES=yes \
    pkg -R "$PKG_CONFIG" -r "$WORK/rootfs" install -y -r FreeBSD-base \
        $BASE_PKGS $BASE_BUILD_PKGS

#
# 3. chroot: install runtime pkgs (pkglist.txt) + build pkgs (buildpkgs.txt)
#    inside the chroot, then purge buildpkgs.txt + buildpkgs-base.txt
#    before the slim/mkuzip pass so build tooling doesn't ship in the ISO.
#    Build pkgs are installed and kept available for any chroot-side build
#    work; only the install/purge plumbing lives here — no inline build
#    recipe (the previous gershwin/GNUstep build was removed).
#
RUNTIME_PKGS=$(grep -v '^[[:space:]]*#' "$ROOT/pkglist.txt"   2>/dev/null | grep -v '^[[:space:]]*$' || true)
BUILD_PKGS=$(  grep -v '^[[:space:]]*#' "$ROOT/buildpkgs.txt" 2>/dev/null | grep -v '^[[:space:]]*$' || true)

if [ -n "$RUNTIME_PKGS" ] || [ -n "$BUILD_PKGS" ]; then
    cp /etc/resolv.conf "$WORK/rootfs/etc/resolv.conf"
    mount -t devfs devfs "$WORK/rootfs/dev"
    cleanup_chroot() {
        umount -f "$WORK/rootfs/dev" 2>/dev/null || true
        rm -f "$WORK/rootfs/etc/resolv.conf"
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
    fi

    # Build pkgs (cmake/ninja/clang/etc.) stay installed through the
    # subsequent build steps (mach.ko, libsystem_kernel, libdispatch).
    # Purge + chroot cleanup move to the very end of the build phase,
    # after libdispatch is built (see "3z" below). Don't call
    # cleanup_chroot here — devfs + resolv.conf must stay live for the
    # libdispatch chroot build to work.
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
# 3c. build libsystem_kernel (formerly libmach) on the host and install
#     it into the chroot under the spike's chosen Apple-Libsystem layout:
#       /usr/lib/libsystem/libsystem_kernel.so + .so.0 sonname symlink
#       /usr/include/mach/mach_traps.h
#       /usr/libdata/pkgconfig/libsystem_kernel.pc
#
echo "==> building libsystem_kernel (src/libmach)"
# bsd.lib.mk's install doesn't auto-create LIBDIR / INCSDIR / FILESDIR;
# pre-create them since /usr/lib/libsystem is our convention (not a
# stock FreeBSD path) so pkgbase doesn't ship it.
mkdir -p "$WORK/rootfs/usr/lib/libsystem" \
         "$WORK/rootfs/usr/include/mach" \
         "$WORK/rootfs/usr/libdata/pkgconfig"
make -C "$ROOT/src/libmach" \
    DESTDIR="$WORK/rootfs" \
    PREFIX=/usr \
    all install
ls -lh "$WORK/rootfs/usr/lib/libsystem/libsystem_kernel.so" \
       "$WORK/rootfs/usr/include/mach/mach_traps.h" \
       "$WORK/rootfs/usr/libdata/pkgconfig/libsystem_kernel.pc"

# ldconfig hint so rtld finds libsystem_kernel.so at runtime. Two parts:
# (a) drop-in file at /usr/local/libdata/ldconfig/freebsd-launchd-mach
#     so /etc/rc.d/ldconfig at boot adds /usr/lib/libsystem to hints.
# (b) ldconfig -m primes /var/run/ld-elf.so.hints inside rootfs.uzip
#     so the live ISO boots with hints already correct under launchd.
echo "==> writing ldconfig hint for /usr/lib/libsystem"
mkdir -p "$WORK/rootfs/usr/local/libdata/ldconfig"
echo "/usr/lib/libsystem" \
    > "$WORK/rootfs/usr/local/libdata/ldconfig/freebsd-launchd-mach"
chroot "$WORK/rootfs" ldconfig -m /usr/lib /usr/lib/libsystem

#
# 3d. build the libsystem_kernel smoke test binary, install to
#     /usr/tests/freebsd-launchd-mach/test_libmach. CI's run.sh invokes
#     it post-login to verify rtld resolution + Mach roundtrip.
#
echo "==> building test_libmach"
mkdir -p "$WORK/rootfs/usr/tests/freebsd-launchd-mach"
cc -I"$WORK/rootfs/usr/include" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_libmach" \
   "$ROOT/src/mach_kmod/tests/test_libmach.c" \
   -lsystem_kernel
ls -lh "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_libmach"

# test_mach_port — exercises the three port-management traps wired
# for the libxpc port (mach_port_allocate / _insert_right / _deallocate).
# Runs after test_libmach in CI's run.sh; failure surfaces as
# MACH-PORT-FAIL.
echo "==> building test_mach_port"
cc -I"$WORK/rootfs/usr/include" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_mach_port" \
   "$ROOT/src/mach_kmod/tests/test_mach_port.c" \
   -lsystem_kernel
ls -lh "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_mach_port"

# test_task_special_port — exercises task_get_special_port /
# task_set_special_port traps (Phase G prerequisite for the bootstrap
# server's port discovery). Failure surfaces as TASK-SPECIAL-PORT-FAIL.
echo "==> building test_task_special_port"
cc -I"$WORK/rootfs/usr/include" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_task_special_port" \
   "$ROOT/src/mach_kmod/tests/test_task_special_port.c" \
   -lsystem_kernel
ls -lh "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_task_special_port"

# Install <servers/bootstrap.h> public header. libsystem_kernel's
# Makefile installs everything else under /usr/include/mach/; the
# bootstrap header sits at /usr/include/servers/bootstrap.h per
# Apple convention. Do it by hand here rather than wrestling with
# bsd.lib.mk INCS_GRP plumbing.
echo "==> installing <servers/bootstrap.h>"
mkdir -p "$WORK/rootfs/usr/include/servers"
cp "$ROOT/src/libmach/include/servers/bootstrap.h" \
   "$WORK/rootfs/usr/include/servers/bootstrap.h"

# test_bootstrap — Phase G1 single-task validation of the bootstrap
# protocol (check_in / look_up round-trip via libbootstrap +
# bootstrap_server_run in a pthread). libbootstrap.c is linked
# statically into the test binary for now; Phase G2 promotes it to
# a real /usr/lib/libsystem/libbootstrap.so once the daemon lands.
echo "==> building test_bootstrap"
cc -I"$WORK/rootfs/usr/include" \
   -I"$ROOT/src/bootstrap" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_bootstrap" \
   "$ROOT/src/bootstrap/tests/test_bootstrap.c" \
   "$ROOT/src/bootstrap/libbootstrap.c" \
   -lsystem_kernel -lpthread
ls -lh "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_bootstrap"

#
# 3e. verify: assert install shape + ldconfig resolution + ldd resolves
#     the test binary's libsystem_kernel.so.0 dep. Build fails fast here
#     instead of producing a broken ISO.
#
echo "==> verifying libsystem_kernel install"
ls -la "$WORK/rootfs/usr/lib/libsystem/" || true
# bsd.lib.mk installs .so.0 as the regular file (the actual library
# binary) and .so as the dev-time symlink pointing to it. Verify both
# shapes correctly.
test -f "$WORK/rootfs/usr/lib/libsystem/libsystem_kernel.so.0" \
    || { echo "FAIL: libsystem_kernel.so.0 (the library binary) missing"; exit 1; }
test -L "$WORK/rootfs/usr/lib/libsystem/libsystem_kernel.so" \
    || { echo "FAIL: libsystem_kernel.so (dev symlink to .so.0) missing"; exit 1; }
test -f "$WORK/rootfs/usr/local/libdata/ldconfig/freebsd-launchd-mach" \
    || { echo "FAIL: ldconfig drop-in missing"; exit 1; }
test ! -e "$WORK/rootfs/usr/local/lib/libmach.so" \
    || { echo "FAIL: old /usr/local/lib/libmach.so still present"; exit 1; }
chroot "$WORK/rootfs" ldconfig -r | grep -q libsystem_kernel \
    || { echo "FAIL: ldconfig hints missing libsystem_kernel"; exit 1; }
chroot "$WORK/rootfs" ldd /usr/tests/freebsd-launchd-mach/test_libmach \
    | grep -q "libsystem_kernel.so.0 => /usr/lib/libsystem/" \
    || { echo "FAIL: ldd doesn't resolve test_libmach to /usr/lib/libsystem/"; exit 1; }
echo "==> libsystem_kernel install verified"

#
# 3f. build libdispatch from src/libdispatch (vendored swift-corelibs-
#     libdispatch + FreeBSD perf patch). cmake/ninja build inside the
#     chroot using buildpkgs already installed (clang, lld, cmake,
#     ninja). Installs:
#       /usr/lib/libsystem/libdispatch.so + .so.0 (the lib)
#       /usr/lib/libsystem/libBlocksRuntime.so + .so.0 (bundled — replaces
#         the dropped FreeBSD-libblocksruntime pkg; same upstream Apple
#         compiler-rt source)
#       /usr/include/dispatch/*.h
#       /usr/include/os/*.h
#       /usr/include/Block.h + Block_private.h
#     Plus libsystem_dispatch / libsystem_blocks symlinks for Apple-
#     canonical naming (per install-layout-spike §4 + §15). Proper
#     SONAME rename to libsystem_dispatch is a future cleanup; symlinks
#     keep both -ldispatch and -lsystem_dispatch link lines working.
#
echo "==> rsyncing src/libdispatch to chroot"
mkdir -p "$WORK/rootfs/tmp/libdispatch"
rsync -a --delete "$ROOT/src/libdispatch/" "$WORK/rootfs/tmp/libdispatch/"

echo "==> building libdispatch in chroot"
chroot "$WORK/rootfs" /bin/sh -ex <<'CHROOT_DISPATCH'
# Chroot inherits caller's PATH which may not include /usr/local/bin
# where pkg installs cmake / ninja. Set explicitly.
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
mkdir -p /tmp/libdispatch-build
cd /tmp/libdispatch-build
cmake -G Ninja /tmp/libdispatch \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib/libsystem \
    -DINSTALL_DISPATCH_HEADERS_DIR=/usr/include/dispatch \
    -DINSTALL_BLOCK_HEADERS_DIR=/usr/include \
    -DINSTALL_OS_HEADERS_DIR=/usr/include/os \
    -DINSTALL_PRIVATE_HEADERS=ON \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release
ninja
ninja install
CHROOT_DISPATCH

echo "==> creating libdispatch / libsystem_dispatch / libsystem_blocks symlinks"
# libdispatch's CMakeLists doesn't set SOVERSION — output is just
# libdispatch.so (unversioned), no .so.0. DT_SONAME is empty, so
# consumers' DT_NEEDED becomes the filename (libdispatch.so).
#
# Issue: FreeBSD's ldconfig only indexes libs matching lib*.so.[0-9]+
# in its hints DB. An unversioned lib*.so is on disk but invisible to
# rtld via the hints lookup. Programs without RPATH/RUNPATH would fail
# to dlopen libdispatch by sonname.
#
# Workaround: create .so.0 symlinks pointing at the unversioned file so
# ldconfig's glob matches. Cleaner long-term fix: patch libdispatch's
# CMakeLists to set SOVERSION + OUTPUT_NAME=system_dispatch.
ln -sf libdispatch.so      "$WORK/rootfs/usr/lib/libsystem/libdispatch.so.0"
ln -sf libBlocksRuntime.so "$WORK/rootfs/usr/lib/libsystem/libBlocksRuntime.so.0"
ln -sf libdispatch.so      "$WORK/rootfs/usr/lib/libsystem/libsystem_dispatch.so"
ln -sf libdispatch.so      "$WORK/rootfs/usr/lib/libsystem/libsystem_dispatch.so.0"
ln -sf libBlocksRuntime.so "$WORK/rootfs/usr/lib/libsystem/libsystem_blocks.so"
ln -sf libBlocksRuntime.so "$WORK/rootfs/usr/lib/libsystem/libsystem_blocks.so.0"

# Re-prime ldconfig hints now that libdispatch + BlocksRuntime are
# installed at /usr/lib/libsystem.
chroot "$WORK/rootfs" ldconfig -m /usr/lib /usr/lib/libsystem

# Cleanup chroot build artifacts.
rm -rf "$WORK/rootfs/tmp/libdispatch" "$WORK/rootfs/tmp/libdispatch-build"

#
# 3g. build the libdispatch smoke test binary, install to
#     /usr/tests/freebsd-launchd-mach/test_libdispatch.
#
echo "==> building test_libdispatch"
cc -I"$WORK/rootfs/usr/include" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_libdispatch" \
   "$ROOT/src/libdispatch-tests/test_libdispatch.c" \
   -ldispatch -lpthread

echo "==> building test_libdispatch_mach"
# Links libsystem_kernel for the userland mach_reply_port / mach_msg
# trap shims the round-trip test uses to allocate a receive port and
# self-send through the kernel.
cc -I"$WORK/rootfs/usr/include" \
   -L"$WORK/rootfs/usr/lib/libsystem" \
   -Wl,-rpath,/usr/lib/libsystem \
   -o "$WORK/rootfs/usr/tests/freebsd-launchd-mach/test_libdispatch_mach" \
   "$ROOT/src/libdispatch-tests/test_libdispatch_mach.c" \
   -ldispatch -lsystem_kernel -lpthread

#
# 3h. verify libdispatch install shape + ldconfig + ldd resolution.
#
echo "==> verifying libdispatch install"
ls -la "$WORK/rootfs/usr/lib/libsystem/" || true
# libdispatch installs unversioned: libdispatch.so is the actual file
# (no .so.0). Same for libBlocksRuntime.
test -f "$WORK/rootfs/usr/lib/libsystem/libdispatch.so" \
    || { echo "FAIL: libdispatch.so (the library binary) missing"; exit 1; }
test -L "$WORK/rootfs/usr/lib/libsystem/libsystem_dispatch.so" \
    || { echo "FAIL: libsystem_dispatch.so symlink missing"; exit 1; }
test -f "$WORK/rootfs/usr/lib/libsystem/libBlocksRuntime.so" \
    || { echo "FAIL: libBlocksRuntime.so missing"; exit 1; }
test -L "$WORK/rootfs/usr/lib/libsystem/libsystem_blocks.so" \
    || { echo "FAIL: libsystem_blocks.so symlink missing"; exit 1; }
test -f "$WORK/rootfs/usr/include/Block.h" \
    || { echo "FAIL: /usr/include/Block.h missing (libdispatch should ship it)"; exit 1; }
test -f "$WORK/rootfs/usr/include/dispatch/dispatch.h" \
    || { echo "FAIL: /usr/include/dispatch/dispatch.h missing"; exit 1; }
chroot "$WORK/rootfs" ldconfig -r | grep -q libdispatch \
    || { echo "FAIL: ldconfig hints missing libdispatch"; exit 1; }
chroot "$WORK/rootfs" ldd /usr/tests/freebsd-launchd-mach/test_libdispatch \
    | grep -q "libdispatch.so => /usr/lib/libsystem/" \
    || { echo "FAIL: ldd doesn't resolve test_libdispatch to /usr/lib/libsystem/"; exit 1; }
echo "==> libdispatch install verified"

#
# 3z. purge build packages + clean pkg cache + tear down chroot.
#     Runs LAST in the build phase, after every chroot-side build
#     (libdispatch) has used cmake/ninja/clang. Build pkgs (cmake/ninja
#     from buildpkgs.txt; clang/lld/-dev from buildpkgs-base.txt) get
#     removed before mkuzip so they don't ship in the ISO. Pkg
#     download cache also cleared.
#
if [ -n "$BUILD_PKGS" ] || [ -n "$BASE_BUILD_PKGS" ]; then
    echo "==> purging build packages (ports + pkgbase build-only)"
    if [ -n "$BUILD_PKGS" ]; then
        # shellcheck disable=SC2086
        chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
            pkg delete -y $BUILD_PKGS
    fi
    if [ -n "$BASE_BUILD_PKGS" ]; then
        # shellcheck disable=SC2086
        chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
            pkg delete -y $BASE_BUILD_PKGS
    fi
    chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
        pkg autoremove -y || true
fi

if [ -n "$RUNTIME_PKGS" ] || [ -n "$BUILD_PKGS" ]; then
    echo "==> cleaning pkg download cache"
    chroot "$WORK/rootfs" env ASSUME_ALWAYS_YES=yes \
        pkg clean -a -y || true

    cleanup_chroot
    trap - EXIT INT TERM
fi

#
# 4. apply local overlays (etc/rc.conf, etc/motd.template,
#    /usr/tests/freebsd-launchd-mach/, ...). No "slim" rm -rf step:
#    pkg curation owns rootfs content end-to-end. If something
#    unwanted shows up, drop the pkg from pkglist-base.txt rather
#    than deleting files post-install.
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

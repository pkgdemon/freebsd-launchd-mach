#!/bin/sh
# make-mach-kmod.sh — build + install mach.ko from src/mach_kmod/.
#
# Standalone wrapper: build.sh calls this at livecd-build time on the
# host (vmactions FreeBSD VM) with SYSDIR pointed at the already-
# extracted /work/freebsd-src/usr/src/sys/. End users on an installed
# FreeBSD with src.txz unpacked can call it with no args.
#
# Usage:
#   make-mach-kmod.sh [--sysdir=PATH] [--prefix=PATH] [--kernbuilddir=PATH]
#
# --sysdir       kernel source tree to build against. Default /usr/src/sys.
# --prefix       DESTDIR for install. Default empty (host /boot/kernel).
# --kernbuilddir opt_*.h override per FreeBSD bsd.kmod.mk. Default: use
#                shipped GENERIC-matching opt_*.h.

set -eu

SYSDIR="/usr/src/sys"
DESTDIR=""
KERNBUILDDIR=""

usage() {
	echo "usage: $0 [--sysdir=PATH] [--prefix=PATH] [--kernbuilddir=PATH]" >&2
	exit 64
}

for arg in "$@"; do
	case "$arg" in
		--sysdir=*)        SYSDIR="${arg#--sysdir=}" ;;
		--prefix=*)        DESTDIR="${arg#--prefix=}" ;;
		--kernbuilddir=*)  KERNBUILDDIR="${arg#--kernbuilddir=}" ;;
		--help|-h)         usage ;;
		*)                 echo "make-mach-kmod: unknown arg: $arg" >&2; usage ;;
	esac
done

# Hard-check kernel source presence.
if [ ! -d "$SYSDIR" ]; then
	echo "make-mach-kmod: SYSDIR ($SYSDIR) not found." >&2
	echo "make-mach-kmod: install src.txz from the matching FreeBSD release," >&2
	echo "  or pass --sysdir=PATH to a kernel source tree you already have." >&2
	exit 1
fi

script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
src_dir="$script_dir/src/mach_kmod"

[ -d "$src_dir/src" ] || {
	echo "make-mach-kmod: $src_dir/src/ not found — wrong script location?" >&2
	exit 1
}

# bsd.kmod.mk works with FreeBSD's base make.
MAKE_ARGS="SYSDIR=$SYSDIR"
if [ -n "$KERNBUILDDIR" ]; then
	MAKE_ARGS="$MAKE_ARGS KERNBUILDDIR=$KERNBUILDDIR"
fi

# shellcheck disable=SC2086
make -C "$src_dir" $MAKE_ARGS

# Install. bsd.kmod.mk's default is KMODDIR=/boot/kernel.
# shellcheck disable=SC2086
make -C "$src_dir" install $MAKE_ARGS DESTDIR="$DESTDIR"

echo "==> make-mach-kmod: mach.ko installed to ${DESTDIR}/boot/kernel/mach.ko"

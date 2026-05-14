# liblaunch — Apple's launch_data IPC library, ported to FreeBSD.
#
# Vendored from apple-oss-distributions/launchd @ launchd-842.92.1.
# Three translation units:
#   liblaunch.c   — the launch_data_t API + launch_msg() Unix-socket IPC
#   libvproc.c    — vproc_t handles, vproc_swap_*, helper-downcall server
#   libbootstrap.c — bootstrap_check_in / _look_up client wrappers
#
# Naming: Apple ships this as liblaunch.dylib at /usr/lib/system/. We
# install liblaunch.so at /usr/lib/libsystem/ per the Phase D layout,
# alongside libsystem_kernel / libdispatch / libxpc.
#
# Build:   cd src/launchd/liblaunch && make
# Install: make DESTDIR=/path/to/rootfs install
#
# Depends at build time on:
#   - the MIG stubs generated in $WORK/launchd-mig (job.h, helper.h,
#     helperServer.h, ...) — build.sh passes MIGOUT=...
#   - our libmach headers (mach/*) — build.sh passes SYSROOT or -I
#   - the FreeBSD shim headers in ../freebsd-shims
#   - launchd's own src/ headers (config.h, ktrace.h)

LIB=		launch
SHLIB_MAJOR=	1

SRCS=		liblaunch.c \
		libvproc.c \
		libbootstrap.c

# MIGOUT — directory holding the mig-generated job.h / helper.h /
# helperServer.h etc. build.sh sets this to $WORK/launchd-mig. On a
# bare standalone build it's empty and the build will fail at the
# first generated #include — expected; liblaunch can't build without
# the MIG output.
MIGOUT?=

# SYSROOT — the in-build rootfs. Earlier build.sh steps install
# headers there that liblaunch needs but that aren't FreeBSD-native:
# <uuid/uuid.h> (libxpc shim), <dispatch/dispatch.h>, <xpc/*.h>,
# <servers/bootstrap.h>, the <mach/mach.h> umbrella. build.sh passes
# SYSROOT=$WORK/rootfs; unset on a bare standalone build.
SYSROOT?=

# Vendored Apple source — don't gate on its warnings.
WARNS?=		0
NO_MAN=		yes

CFLAGS+=	-I${.CURDIR}			# launch.h, vproc.h, ... (own headers)
CFLAGS+=	-I${.CURDIR}/../src		# config.h, ktrace.h
CFLAGS+=	-I${.CURDIR}/../freebsd-shims	# libkern/, os/, libproc.h, ...
CFLAGS+=	-I${.CURDIR}/../../libmach/include	# mach/*
.if !empty(MIGOUT)
CFLAGS+=	-I${MIGOUT}			# job.h, helper.h, helperServer.h
.endif
.if !empty(SYSROOT)
CFLAGS+=	-I${SYSROOT}${PREFIX}/include	# uuid/, dispatch/, xpc/, servers/
LDADD+=		-L${SYSROOT}${PREFIX}/lib/libsystem
.endif

# launchd-842 expects these Apple build-time defines (see xcconfigs in
# the upstream tree). __APPLE_API_PRIVATE / PRIVATE unlock the private
# launch_priv.h / vproc_priv.h surface the three .c files use.
CFLAGS+=	-D__APPLE_API_PRIVATE -DPRIVATE=1
CFLAGS+=	-fblocks			# libdispatch block syntax

LIBDIR=		${PREFIX}/lib/libsystem

PREFIX?=	/usr

.include <bsd.lib.mk>

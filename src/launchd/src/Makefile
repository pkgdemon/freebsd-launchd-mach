# launchd — Apple's launchd daemon, ported to FreeBSD.
#
# Vendored from apple-oss-distributions/launchd @ launchd-842.92.1.
# Seven hand-written translation units (launchd.c core.c runtime.c
# ipc.c log.c kill2.c ktrace.c) plus the MIG-generated RPC stubs for
# the job / job_forward / job_reply / internal / helper / mach_exc /
# notify subsystems.
#
# Naming: Apple ships /sbin/launchd as PID 1. We are NOT PID 1 yet —
# this Phase I1c build only proves launchd compiles, links, and execs
# the no-IPC CLI path (LAUNCHD-BUILD-OK marker). Installed at
# /usr/local/sbin/launchd to stay clear of FreeBSD's /sbin/init.
#
# Build:   cd src/launchd/src && make
# Install: make DESTDIR=/path/to/rootfs install
#
# Depends at build time on:
#   - the MIG stubs generated in $WORK/launchd-mig — build.sh passes
#     MIGOUT=...; .PATH below pulls the *User.c / *Server.c into SRCS
#   - liblaunch (built by the prior step, installed in SYSROOT)
#   - our libmach / libxpc / libdispatch headers + libs (SYSROOT)
#   - the FreeBSD shim headers in ../freebsd-shims
#   - launchd's own liblaunch/ public headers (launch.h, vproc.h, ...)

PROG=		launchd
MAN=

# MIGOUT — directory holding the mig-generated *User.c / *Server.c /
# *.h. build.sh sets this to $WORK/launchd-mig. Empty on a bare
# standalone build — the build then fails at the first generated
# #include, which is expected; launchd can't build without MIG output.
MIGOUT?=

# SYSROOT — the in-build rootfs. Earlier build.sh steps install there
# the headers + libs launchd needs but that aren't FreeBSD-native:
# liblaunch.so, libxpc.so, libdispatch.so, libsystem_kernel.so, plus
# <uuid/uuid.h>, <dispatch/*>, <xpc/*>, <servers/bootstrap.h>, the
# <mach/mach.h> umbrella. build.sh passes SYSROOT=$WORK/rootfs.
SYSROOT?=

SRCS=		launchd.c \
		core.c \
		runtime.c \
		ipc.c \
		log.c \
		kill2.c \
		ktrace.c

# MIG-generated stubs. launchd is both client and server for the
# job / internal / helper subsystems, server-only for mach_exc and
# notify, and client-only for job_reply / job_forward — but the MIG
# step emits both *User.c and *Server.c for every .defs, so just
# compile them all. Unused stubs are dead code, not link conflicts.
.if !empty(MIGOUT)
.PATH:		${MIGOUT}
SRCS+=		jobUser.c jobServer.c \
		job_forwardUser.c job_forwardServer.c \
		job_replyUser.c job_replyServer.c \
		internalUser.c internalServer.c \
		helperUser.c helperServer.c \
		mach_excUser.c mach_excServer.c \
		notifyUser.c notifyServer.c
.endif

# Vendored Apple source — don't gate on its warnings.
WARNS?=		0
NO_MAN=		yes

CFLAGS+=	-I${.CURDIR}			# config.h, core.h, launchd.h, ...
CFLAGS+=	-I${.CURDIR}/../liblaunch	# launch.h, vproc.h, bootstrap.h, ...
CFLAGS+=	-I${.CURDIR}/../freebsd-shims	# libkern/, os/, libproc.h, ...
CFLAGS+=	-I${.CURDIR}/../../libmach/include	# mach/*
.if !empty(MIGOUT)
CFLAGS+=	-I${MIGOUT}			# job.h, jobServer.h, mach_excServer.h, ...
.endif
.if !empty(SYSROOT)
CFLAGS+=	-I${SYSROOT}/usr/include	# uuid/, dispatch/, xpc/, servers/
LDADD+=		-L${SYSROOT}/usr/lib/libsystem
LDFLAGS+=	-Wl,-rpath,/usr/lib/libsystem
.endif

# launchd-842 expects these Apple build-time defines.
CFLAGS+=	-D__APPLE_API_PRIVATE -DPRIVATE=1
CFLAGS+=	-fblocks			# libdispatch block syntax

LDADD+=		-llaunch -lxpc -ldispatch -lsystem_kernel -lpthread

BINDIR=		/usr/local/sbin

.include <bsd.prog.mk>

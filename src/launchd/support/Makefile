# launchctl — Apple's launchd user-side control utility, ported.
#
# Vendored from apple-oss-distributions/launchd @ launchd-842.92.1.
# Single 4549-LOC translation unit (launchctl.c) plus shims under
# ../freebsd-shims for macOS-specific headers (IOKit, NSSystemDirectories,
# mach-o/getsect, libinfo, asl, os, ...).
#
# Install path: /bin/launchctl, matching Apple's shipping layout
# (/bin/launchctl on macOS too).
#
# Build:   cd src/launchd/support && make
# Install: make DESTDIR=/path/to/rootfs install
#
# Depends at build time on:
#   - liblaunch (built by the prior step, installed in SYSROOT)
#   - libCoreFoundation + lib_FoundationICU (ICU.x epic, installed in SYSROOT)
#   - libxpc / libdispatch / libsystem_kernel (installed in SYSROOT)
#   - the freebsd-shims headers in ../freebsd-shims
#   - launchd's own liblaunch/ public headers (launch.h, vproc.h, ...)

PROG=		launchctl
MAN=

SRCS=		launchctl.c

# Vendored Apple source — don't gate on its warnings.
WARNS?=		0
NO_MAN=		yes

CFLAGS+=	-I${.CURDIR}/../liblaunch	# launch.h, vproc.h, bootstrap.h, ...
CFLAGS+=	-I${.CURDIR}/../src		# config.h shared with the daemon
CFLAGS+=	-I${.CURDIR}/../freebsd-shims	# IOKit/, mach-o/, NSSystemDirectories.h, ...
CFLAGS+=	-I${.CURDIR}/../../libmach/include	# mach/*
.if !empty(SYSROOT)
CFLAGS+=	-I${SYSROOT}/usr/include	# CoreFoundation/, _foundation_unicode/, dispatch/, xpc/, ...
# libedit's readline-compatible API. FreeBSD base ships libedit with a
# <readline/readline.h> header at /usr/include/edit/readline/readline.h.
# launchctl.c uses readline only for the interactive `launchctl` REPL;
# linking libedit's compat layer satisfies the symbols at link time.
CFLAGS+=	-I${SYSROOT}/usr/include/edit
LDADD+=		-L${SYSROOT}/usr/lib/libsystem
LDFLAGS+=	-Wl,-rpath,/usr/lib/libsystem
.endif

# launchd-842 expects these Apple build-time defines.
CFLAGS+=	-D__APPLE_API_PRIVATE -DPRIVATE=1
# CF + ICU consumer flags. MUST match what libicu and libCoreFoundation
# were built with — see icu_complete_corefoundation_ok memory entry:
#   __HAS_APPLE_ICU__=1   activate Apple-private ICU code paths
#   U_DISABLE_RENAMING=1  match libicu's symbol export scheme
#   APPLE_ICU_CHANGES=1   expose Apple-private ICU function decls
CFLAGS+=	-D__HAS_APPLE_ICU__=1
CFLAGS+=	-DU_DISABLE_RENAMING=1
CFLAGS+=	-DAPPLE_ICU_CHANGES=1
CFLAGS+=	-fblocks			# CF + libdispatch block syntax
CFLAGS+=	-fconstant-cfstrings		# CFSTR() literal -> CFStringRef

# launchctl's link line. -lCoreFoundation pulls in the CF runtime.
# -l_FoundationICU is the swift-foundation-icu unified library
# (libicucore.so symlink would also work).
LDADD+=		-llaunch -lxpc -ldispatch -lsystem_kernel \
		-lCoreFoundation -l_FoundationICU \
		-lpthread -lutil -lBlocksRuntime \
		-ledit		# readline-compat layer for the interactive REPL

BINDIR=		/bin

.include <bsd.prog.mk>

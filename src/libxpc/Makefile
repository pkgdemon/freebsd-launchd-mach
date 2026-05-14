# libxpc — Apple-canonical XPC userland library, ported from ravynOS's
# lib/libxpc/. Uses our libsystem_kernel for Mach trap access and our
# vendored libdispatch for queue/source support. The bootstrap_check_in
# / bootstrap_look_up dependencies are satisfied by libbootstrap (in
# src/bootstrap/), which is built into both the daemon and the smoke
# tests for now and will become a real /usr/lib/libsystem/libbootstrap.so
# once it stabilizes.
#
# Naming: matches Apple's libxpc.dylib on macOS so Apple-source code that
# links -lxpc compiles unchanged. Install path follows the Phase D
# layout — /usr/lib/libsystem/libxpc.so. Headers at /usr/include/xpc/.
#
# Vendored from ravynOS, which itself imported from NextBSD. Local subr_
# nvlist.c + subr_nvpair.c carry their own copies of FreeBSD libnv (the
# only consumer of libnv in this stack), so we don't need to link -lnv;
# the symbols are statically rolled in.
#
# Build:   cd src/libxpc && make
# Install: make DESTDIR=/path/to/rootfs install

LIB=		xpc
SHLIB_MAJOR=	4

# Vendored bootstrap protocol (libbootstrap.c) is compiled in directly
# so libxpc.so resolves bootstrap_check_in / bootstrap_look_up without
# needing a separate libbootstrap.so on disk. The same .c is also
# compiled directly into bootstrap_server + the smoke tests; no symbol
# conflict because those binaries don't link libxpc. A future cleanup
# commit can promote libbootstrap to its own .so once its API is
# stable, at which point libxpc and the daemon/tests all link -lbootstrap.
.PATH:		${.CURDIR}/../bootstrap

SRCS=		xpc_dictionary.c \
		xpc_misc.c \
		xpc_type.c \
		xpc_array.c \
		xpc_connection.c \
		subr_nvlist.c \
		subr_nvpair.c \
		msgio.c \
		stubs.c \
		libbootstrap.c

# SYSROOT — points at the in-build rootfs so we can find the headers
# libxpc consumes (mach/*, dispatch/*, servers/bootstrap.h) and the
# libs it links against (libsystem_kernel, libdispatch). build.sh
# passes SYSROOT=$WORK/rootfs; unset on standalone host build.
SYSROOT?=

CFLAGS+=	-I${.CURDIR} -I${.CURDIR}/../bootstrap
CFLAGS+=	-fblocks
.if !empty(SYSROOT)
CFLAGS+=	-I${SYSROOT}${PREFIX}/include
LDADD+=		-L${SYSROOT}${PREFIX}/lib/libsystem
.endif
NO_MAN=		yes
WARNS?=		0	# vendored code; don't gate on style warnings

INCS=		xpc/activity.h \
		xpc/base.h \
		xpc/connection.h \
		xpc/debug.h \
		xpc/endpoint.h \
		xpc/launchd.h \
		xpc/xpc.h
INCSDIR=	${PREFIX}/include/xpc
LIBDIR=		${PREFIX}/lib/libsystem

# Link against libsystem_kernel for Mach trap shims and libdispatch
# for queue/source primitives. Both are at /usr/lib/libsystem/ per
# the Phase D layout. -lBlocksRuntime resolves _Block_copy /
# _NSConcreteStackBlock etc. from -fblocks; -lsbuf resolves the
# sbuf_* API xpc_misc.c uses for xpc_copy_description. -rpath lets
# rtld find our libsystem dir at runtime.
LDADD+=		-lsystem_kernel -ldispatch -lBlocksRuntime -lsbuf -lpthread
LDADD+=		-Wl,-rpath,${PREFIX}/lib/libsystem

PREFIX?=	/usr

.include <bsd.lib.mk>

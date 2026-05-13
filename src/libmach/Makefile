# libsystem_kernel — userland C shim that turns the mach.ko-registered
# syscalls into named functions (mach_reply_port, mach_task_self, ...).
#
# Naming: matches Apple's libsystem_kernel.dylib on macOS so Apple-source
# code that links -lsystem_kernel works unchanged. See spike doc §3.
#
# Build:   cd src/libmach && make
# Install: make DESTDIR=/path/to/rootfs install
#   produces:
#     ${DESTDIR}/usr/lib/libsystem/libsystem_kernel.so
#     ${DESTDIR}/usr/lib/libsystem/libsystem_kernel.so.0
#     ${DESTDIR}/usr/include/mach/mach_traps.h
#     ${DESTDIR}/usr/include/mach/message.h
#     ${DESTDIR}/usr/libdata/pkgconfig/libsystem_kernel.pc

LIB=		system_kernel
SHLIB_MAJOR=	0
SRCS=		mach_traps.c

CFLAGS+=	-I${.CURDIR}/include
NO_MAN=		yes
WARNS?=		6

INCS=		include/mach/mach_traps.h \
		include/mach/message.h
INCSDIR=	${PREFIX}/include/mach
LIBDIR=		${PREFIX}/lib/libsystem

# pkg-config file: consumers do `pkg-config --libs libsystem_kernel`
# and get the right -L and -l flags for /usr/lib/libsystem/.
FILES=		libsystem_kernel.pc
FILESDIR=	${PREFIX}/libdata/pkgconfig

PREFIX?=	/usr

.include <bsd.lib.mk>

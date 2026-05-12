# libmach — userland C shim that turns the mach.ko-registered syscalls
# into named functions (mach_reply_port, mach_task_self, ...).
#
# Build: cd src/libmach && make
# Install: make install (puts libmach.so in /usr/local/lib, headers in
# /usr/local/include/mach).

LIB=		mach
SHLIB_MAJOR=	0
SRCS=		mach_traps.c

CFLAGS+=	-I${.CURDIR}/include
NO_MAN=		yes
WARNS?=		6

INCS=		include/mach/mach_traps.h
INCSDIR=	${PREFIX}/include/mach
LIBDIR=		${PREFIX}/lib

PREFIX?=	/usr/local

.include <bsd.lib.mk>

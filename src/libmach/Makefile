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
#     ${DESTDIR}/usr/include/mach/mach_traps_mux.h
#     ${DESTDIR}/usr/include/mach/mach_port.h
#     ${DESTDIR}/usr/include/mach/message.h
#     ${DESTDIR}/usr/include/mach/task_special_ports.h
#     ${DESTDIR}/usr/include/mach/host_special_ports.h
#     ${DESTDIR}/usr/libdata/pkgconfig/libsystem_kernel.pc

LIB=		system_kernel
SHLIB_MAJOR=	0
SRCS=		mach_traps.c

CFLAGS+=	-I${.CURDIR}/include
NO_MAN=		yes
WARNS?=		6

# <mach/mach.h> (the Apple-canonical umbrella header) is INTENTIONALLY
# absent from INCS. libdispatch's source build auto-detects via
# `__has_include(<mach/mach.h>)` and turns on HAVE_MACH if present,
# which then pulls in Apple-internal types (mach_msg_overwrite,
# mach_voucher_t, OS_NONNULL* attrs, dispatch_mig_callback_t, ...)
# that our freebsd-launchd-mach stack doesn't provide. Installing
# the umbrella before libdispatch builds breaks libdispatch's build.
# build.sh installs <mach/mach.h> manually *after* libdispatch is
# done (in the libxpc step) — same pattern as <servers/bootstrap.h>.
INCS=		include/mach/mach_traps.h \
		include/mach/mach_traps_mux.h \
		include/mach/mach_port.h \
		include/mach/message.h \
		include/mach/task_special_ports.h \
		include/mach/host_special_ports.h \
		include/mach/boolean.h \
		include/mach/kern_return.h \
		include/mach/std_types.h \
		include/mach/ndr.h \
		include/mach/machine.h \
		include/mach/vm_map.h \
		include/mach/clock_types.h \
		include/mach/port.h
INCSDIR=	${PREFIX}/include/mach
LIBDIR=		${PREFIX}/lib/libsystem

# pkg-config file: consumers do `pkg-config --libs libsystem_kernel`
# and get the right -L and -l flags for /usr/lib/libsystem/.
FILES=		libsystem_kernel.pc
FILESDIR=	${PREFIX}/libdata/pkgconfig

PREFIX?=	/usr

.include <bsd.lib.mk>

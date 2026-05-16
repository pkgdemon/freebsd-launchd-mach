# hwregd — freebsd-launchd-mach hardware registry daemon.
#
# Phase 0 / iter 1: minimum viable /dev/devctl reader. Builds to
# /usr/sbin/hwregd, runs as a launchd-managed daemon under PID-1
# launchd (plist at /System/Library/LaunchDaemons/org.freebsd.hwregd.plist).
#
# No external library dependencies yet -- pure libc. Future iters
# will pull in libsystem_kernel (Mach IPC) and lift devmatch's
# search_hints() parser.
#
# Build:   cd src/hwregd && make
# Install: make DESTDIR=/path/to/rootfs install

PROG=		hwregd
MAN=
SRCS=		hwregd.c

WARNS?=		6
NO_MAN=		yes

CFLAGS+=	-O2 -pipe
CFLAGS+=	-Wall -Wextra -Wshadow -Wstrict-prototypes
CFLAGS+=	-Wmissing-prototypes -Wpointer-arith

BINDIR=		/usr/sbin

.include <bsd.prog.mk>

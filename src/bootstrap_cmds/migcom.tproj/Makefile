# migcom — Apple's Mach Interface Generator compiler backend, ported to FreeBSD.
#
# Vendored verbatim from apple-oss-distributions/bootstrap_cmds (no source
# patches yet — pure C + yacc + lex). The user-facing `mig` is the shell
# wrapper mig.sh (installed as /usr/bin/mig); migcom is the actual compiler
# this Makefile builds, installed at /usr/libexec/migcom.
#
# Build deps from FreeBSD base: byacc + flex (both ship in FreeBSD-utilities
# / FreeBSD-yacc / FreeBSD-flex pkgbase pkgs).
#
# Build:   cd src/bootstrap_cmds/migcom.tproj && make
# Install: make DESTDIR=/path/to/rootfs install

PROG=		migcom
NO_MAN=		yes		# man pages installed separately (mig.1, migcom.1)
WARNS?=		0		# vendored code; don't gate on style warnings

# Yacc: -d emits the token header. Don't pass -o — FreeBSD's bsd.prog.mk
# .y.c transformation already renames the yacc output to ${.PREFIX}.c
# (parser.c) and, with -d, emits ${.PREFIX}.h (parser.h). Overriding -o
# fights that rule. lexxer.l #includes "parser.h" to pick up the tokens;
# parser.h must therefore exist before lexxer.c compiles (see the
# explicit dependency near the bottom of this Makefile).
YFLAGS=		-d

# Source list matches Apple's mig.xcodeproj "Sources" build phase
# exactly. NB: handler.c is present in the repo but is NOT in Apple's
# build phase — it's stale dead code (references removed MIG concepts
# like IsCamelot / itLongForm / itDeallocate) and must not be compiled.
SRCS=		parser.y \
		lexxer.l \
		error.c \
		global.c \
		header.c \
		mig.c \
		routine.c \
		server.c \
		statement.c \
		string.c \
		type.c \
		user.c \
		utils.c

# bsd.prog.mk inserts ${.OBJDIR} into the include path automatically,
# which lets the generated parser.c / lexxer.c find each other's headers.
CFLAGS+=	-I${.CURDIR} -I${.OBJDIR}

# migcom's sources #include <mach/{message,boolean,kern_return,std_types,
# ndr}.h>. On FreeBSD those don't live in the host /usr/include — they're
# our libmach headers. Point at the in-tree libmach include dir so the
# host build resolves them without needing libmach installed first.
CFLAGS+=	-I${.CURDIR}/../../libmach/include

# Apple's Xcode build defines this; mig.c / mig_machine.h reference it for
# bit-width selection. Default to 1 (matches macOS); irrelevant for our
# launchd-842 use since we only target 64-bit.
CFLAGS+=	-DMIG_TYPE_CHECK=1

BINDIR=		/usr/libexec	# canonical Apple path

PREFIX?=	/usr

.include <bsd.prog.mk>

# lexxer.l #includes "parser.h" (the yacc -d token header). Without an
# explicit dependency, make may compile lexxer.c before parser.y has
# been processed. Force the ordering for every lexxer object variant
# bsd.prog.mk might build (.o / .pieo / .pico).
lexxer.o lexxer.pieo lexxer.pico: parser.h

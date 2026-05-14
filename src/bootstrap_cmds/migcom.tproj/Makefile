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

# Yacc generates parser.c + parser.h. The generated parser uses yylex /
# yyparse — matches the lexxer.l output. -d emits the header so other
# sources can pull token definitions in.
YFLAGS=		-d -o parser.c

SRCS=		parser.y \
		lexxer.l \
		error.c \
		global.c \
		handler.c \
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

# Apple's Xcode build defines this; mig.c / mig_machine.h reference it for
# bit-width selection. Default to 1 (matches macOS); irrelevant for our
# launchd-842 use since we only target 64-bit.
CFLAGS+=	-DMIG_TYPE_CHECK=1

BINDIR=		/usr/libexec	# canonical Apple path

PREFIX?=	/usr

.include <bsd.prog.mk>

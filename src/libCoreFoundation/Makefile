# libCoreFoundation — swift-corelibs-foundation's pure-C CoreFoundation,
# built standalone (non-Swift mode) for the freebsd-launchd-mach
# system-services lane.
#
# Vendored from github.com/swiftlang/swift-corelibs-foundation at
# upstream SHA 0e20e4a (2026-05-13). License: Apache 2.0 (see LICENSE).
#
# Build with DEPLOYMENT_RUNTIME_SWIFT=0 (patched in
# internalInclude/CoreFoundation_Prefix.h) — the legacy bitfield-CAS
# refcount path. Avoids the libswiftCore.so transitive dependency that
# would otherwise be pulled into every CF-linking system service.
#
# Drops 16 ICU-consuming .c files (CFLocale*, CFCalendar*,
# CFDateFormatter, CFNumberFormatter, CFListFormatter,
# CFDateIntervalFormatter, CFRelativeDateTimeFormatter,
# CFStringTransform, CFRegularExpression, CFCharacterSet,
# CFStringEncodingDatabase, CFICUConverters, CFApplicationPreferences)
# per the ICU audit at
# pkgdemon.github.io/freebsd-libcorefoundation-icu-audit.html. Also
# drops 2 Windows-only files (CFWindowsUtilities,
# CFTimeZone_WindowsMapping). All dropped files remain in the tree as
# upstream reference; flipping back is a SRCS change here, not a git
# revert.
#
# Build:   cd src/libCoreFoundation && make
# Install: make DESTDIR=/path/to/rootfs install
#
# Install layout (per the install-layout spike):
#   /usr/lib/libsystem/libCoreFoundation.so.6
#   /usr/lib/libsystem/libCoreFoundation.so  (symlink)
#   /usr/include/CoreFoundation/*.h
#
# Depends at build time on:
#   - libdispatch + Block.h + libBlocksRuntime (built by the prior
#     libdispatch step, installed in SYSROOT)
#   - libpthread, libc, libdl (FreeBSD base)
#   - libmach/include/mach/*.h for the Mach surface (CFMachPort,
#     CFMessagePort)

LIB=		CoreFoundation
SHLIB_MAJOR=	6

# SYSROOT — the in-build rootfs. build.sh passes SYSROOT=$WORK/rootfs;
# unset on a bare standalone build. Carries libdispatch headers (Block.h,
# dispatch/*), libxpc headers, and the libmach Mach surface.
SYSROOT?=

# 68 source files — full upstream minus 16 ICU files minus 2 Windows files.
SRCS=		CFArray.c \
		CFAttributedString.c \
		CFBag.c \
		CFBase.c \
		CFBasicHash.c \
		CFBigNumber.c \
		CFBinaryHeap.c \
		CFBinaryPList.c \
		CFBitVector.c \
		CFBuiltinConverters.c \
		CFBundle_Binary.c \
		CFBundle_DebugStrings.c \
		CFBundle_Executable.c \
		CFBundle_Grok.c \
		CFBundle_InfoPlist.c \
		CFBundle_Locale.c \
		CFBundle_Main.c \
		CFBundle_ResourceFork.c \
		CFBundle_Resources.c \
		CFBundle_SplitFileName.c \
		CFBundle_Strings.c \
		CFBundle_Tables.c \
		CFBundle.c \
		CFBurstTrie.c \
		CFConcreteStreams.c \
		CFData.c \
		CFDate.c \
		CFDateComponents.c \
		CFDateInterval.c \
		CFDictionary.c \
		CFError.c \
		CFFileUtilities.c \
		CFKnownLocations.c \
		CFNumber.c \
		CFOldStylePList.c \
		CFPlatform.c \
		CFPlatformConverters.c \
		CFPlugIn.c \
		CFPreferences.c \
		CFPropertyList.c \
		CFRunArray.c \
		CFRunLoop.c \
		CFRuntime.c \
		CFSet.c \
		CFSocket.c \
		CFSocketStream.c \
		CFSortFunctions.c \
		CFStorage.c \
		CFStream.c \
		CFString.c \
		CFStringEncodingConverter.c \
		CFStringEncodings.c \
		CFStringScanner.c \
		CFStringUtilities.c \
		CFSystemDirectories.c \
		CFTimeZone.c \
		CFTree.c \
		CFUniChar.c \
		CFUnicodeDecomposition.c \
		CFUnicodePrecomposition.c \
		CFURL.c \
		CFURLAccess.c \
		CFURLComponents_URIParser.c \
		CFURLComponents.c \
		CFUtilities.c \
		CFUUID.c \
		CFXMLPreferencesDomain.c \
		uuid.c

# Vendored Apple source — don't gate on its warnings.
WARNS?=		0
NO_MAN=		yes

CFLAGS+=	-I${.CURDIR}/include			# Public CF headers
CFLAGS+=	-I${.CURDIR}/internalInclude		# Internal CF headers

# Force-include the CF prefix header. Upstream's CMakeLists registers it
# via target_precompile_headers(CoreFoundation PRIVATE ...); the prefix
# defines CF_PRIVATE, DEPLOYMENT_RUNTIME_SWIFT, CF_INLINE, TARGET_OS_*,
# the strlcpy/asprintf compat shims, etc., and only CFStringTransform.c
# explicitly #includes it. Force-include is the bsd.lib.mk analogue.
CFLAGS+=	-include ${.CURDIR}/internalInclude/CoreFoundation_Prefix.h
.if !empty(SYSROOT)
CFLAGS+=	-I${SYSROOT}/usr/include		# Block.h, dispatch/*, mach/*
LDADD+=		-L${SYSROOT}/usr/lib/libsystem
LDFLAGS+=	-Wl,-rpath,/usr/lib/libsystem
.endif

# CF build-time defines.
#   CF_BUILDING_CF — flips macros for "we ARE CF" vs "we use CF"
#   DEPLOYMENT_TARGET_FREEBSD — platform identifier in CFPlatform.c branches
#   __HAS_DISPATCH__ — libdispatch is available (Block.h, dispatch/dispatch.h)
#   _GNU_SOURCE — pull in non-POSIX libc surface CF uses (RTLD_DEFAULT, etc.)
CFLAGS+=	-DCF_BUILDING_CF=1
CFLAGS+=	-DDEPLOYMENT_TARGET_FREEBSD=1
CFLAGS+=	-D__HAS_DISPATCH__=1
CFLAGS+=	-D_GNU_SOURCE=1
CFLAGS+=	-fblocks
CFLAGS+=	-fconstant-cfstrings

LDADD+=		-ldispatch -lBlocksRuntime -lpthread

LIBDIR=		${PREFIX}/lib/libsystem

PREFIX?=	/usr

# Headers — install the entire public include/ subtree to
# /usr/include/CoreFoundation/. Apple-source consumers do
# #include <CoreFoundation/CFFoo.h>.
INCSDIR=	${PREFIX}/include/CoreFoundation

# Just enumerate the public headers explicitly (bsd.lib.mk's INCS
# expects a list, not a glob). The internalInclude/ headers stay
# private to the build.
INCS=		include/CFArray.h \
		include/CFAttributedString.h \
		include/CFAttributedStringPriv.h \
		include/CFAvailability.h \
		include/CFBag.h \
		include/CFBase.h \
		include/CFBigNumber.h \
		include/CFBinaryHeap.h \
		include/CFBitVector.h \
		include/CFBundle_BinaryTypes.h \
		include/CFBundle.h \
		include/CFBundlePriv.h \
		include/CFBurstTrie.h \
		include/CFByteOrder.h \
		include/CFCalendar.h \
		include/CFCalendarPriv.h \
		include/CFCharacterSet.h \
		include/CFCharacterSetPriv.h \
		include/CFConstantKeys.h \
		include/CFData.h \
		include/CFDate.h \
		include/CFDateComponents.h \
		include/CFDateFormatter_Private.h \
		include/CFDateFormatter.h \
		include/CFDateInterval.h \
		include/CFDateIntervalFormatter.h \
		include/CFDictionary.h \
		include/CFError_Private.h \
		include/CFError.h \
		include/CFKnownLocations.h \
		include/CFListFormatter.h \
		include/CFLocale_Private.h \
		include/CFLocale.h \
		include/CFLocking.h \
		include/CFLogUtilities.h \
		include/CFMachPort.h \
		include/CFMessagePort.h \
		include/CFNotificationCenter.h \
		include/CFNumber_Private.h \
		include/CFNumber.h \
		include/CFNumberFormatter.h \
		include/CFOverflow.h \
		include/CFPlugIn.h \
		include/CFPlugInCOM.h \
		include/CFPreferences.h \
		include/CFPriv.h \
		include/CFPropertyList_Private.h \
		include/CFPropertyList.h \
		include/CFRegularExpression.h \
		include/CFRelativeDateTimeFormatter.h \
		include/CFRunArray.h \
		include/CFRunLoop.h \
		include/CFRuntime.h \
		include/CFSet.h \
		include/CFSocket.h \
		include/CFStorage.h \
		include/CFStream.h \
		include/CFStreamAbstract.h \
		include/CFStreamPriv.h \
		include/CFString_Private.h \
		include/CFString.h \
		include/CFStringDefaultEncoding.h \
		include/CFStringEncodingConverter.h \
		include/CFStringEncodingConverterExt.h \
		include/CFStringEncodingConverterPriv.h \
		include/CFStringEncodingDatabase.h \
		include/CFStringEncodingExt.h \
		include/CFTargetConditionals.h \
		include/CFTimeZone.h \
		include/CFTree.h \
		include/CFUniChar.h \
		include/CFUnicodeDecomposition.h \
		include/CFUnicodePrecomposition.h \
		include/CFURL.h \
		include/CFURLAccess.h \
		include/CFURLComponents.h \
		include/CFURLPriv.h \
		include/CFUserNotification.h \
		include/CFUtilities.h \
		include/CFUUID.h \
		include/CoreFoundation.h \
		include/ForFoundationOnly.h \
		include/ForSwiftFoundationOnly.h

.include <bsd.lib.mk>

##===----------------------------------------------------------------------===##
##
## This source file is part of the SwiftFoundation open source project
##
## Copyright (c) 2024 Apple Inc. and the SwiftFoundation project authors
## Licensed under Apache License v2.0
##
## See LICENSE.txt for license information
## See CONTRIBUTORS.md for the list of SwiftFoundation project authors
##
## SPDX-License-Identifier: Apache-2.0
##
##===----------------------------------------------------------------------===##

target_include_directories(_FoundationICU PRIVATE .)
target_sources(_FoundationICU
    PRIVATE
        aaplbfct.cpp
        appendable.cpp
        bmpset.cpp
        brkeng.cpp
        brkiter.cpp
        bytesinkutil.cpp
        bytestream.cpp
        bytestrie.cpp
        bytestriebuilder.cpp
        bytestrieiterator.cpp
        caniter.cpp
        characterproperties.cpp
        chariter.cpp
        charstr.cpp
        cmemory.cpp
        cstr.cpp
        cstring.cpp
        cwchar.cpp
        dictbe.cpp
        dictionarydata.cpp
        dtintrv.cpp
        edits.cpp
        emojiprops.cpp
        errorcode.cpp
        filteredbrk.cpp
        filterednormalizer2.cpp
        # freebsd-launchd-mach patch (2026-05-15): icu_packaged_data.cpp
        # is removed from sources and replaced by freebsd_icu_packaged_data.S
        # (added at the end of this file). The original .cpp #includes 4 x
        # 50 MB hex chunk files into one TU, peaking at 6-10 GB resident
        # in clang's parser. The .S variant uses .incbin to embed a binary
        # blob (decoded from the same chunks at build time), peaking at
        # ~150 MB. See the assembler file's header comment for details.
        icudataver.cpp
        icuplug.cpp
        loadednormalizer2impl.cpp
        localebuilder.cpp
        localematcher.cpp
        localeprioritylist.cpp
        localsvc.cpp
        locavailable.cpp
        locbased.cpp
        locdispnames.cpp
        locdistance.cpp
        locdspnm.cpp
        locid.cpp
        loclikely.cpp
        loclikelysubtags.cpp
        locmap.cpp
        locresdata.cpp
        locutil.cpp
        lsr.cpp
        lstmbe.cpp
        messagepattern.cpp
        mlbe.cpp
        normalizer2.cpp
        normalizer2impl.cpp
        normlzr.cpp
        parsepos.cpp
        patternprops.cpp
        pluralmap.cpp
        propname.cpp
        propsvec.cpp
        punycode.cpp
        putil.cpp
        rbbi.cpp
        rbbi57.cpp
        rbbi_cache.cpp
        rbbidata.cpp
        rbbidata57.cpp
        rbbinode.cpp
        rbbirb.cpp
        rbbirb57.cpp
        rbbiscan.cpp
        rbbiscan57.cpp
        rbbisetb.cpp
        rbbisetb57.cpp
        rbbistbl.cpp
        rbbistbl57.cpp
        rbbitblb.cpp
        rbbitblb57.cpp
        rbtok.cpp
        resbund.cpp
        resbund_cnv.cpp
        resource.cpp
        restrace.cpp
        ruleiter.cpp
        schriter.cpp
        serv.cpp
        servlk.cpp
        servlkf.cpp
        servls.cpp
        servnotf.cpp
        servrbf.cpp
        servslkf.cpp
        sharedobject.cpp
        simpleformatter.cpp
        static_unicode_sets.cpp
        stringpiece.cpp
        stringtriebuilder.cpp
        ualoc.cpp
        uarrsort.cpp
        ubidi.cpp
        ubidi_props.cpp
        ubidiln.cpp
        ubiditransform.cpp
        ubidiwrt.cpp
        ubrk.cpp
        ucase.cpp
        ucasemap.cpp
        ucasemap_titlecase_brkiter.cpp
        ucat.cpp
        uchar.cpp
        ucharstrie.cpp
        ucharstriebuilder.cpp
        ucharstrieiterator.cpp
        uchriter.cpp
        ucln_cmn.cpp
        ucmndata.cpp
        ucnv.cpp
        ucnv2022.cpp
        ucnv_bld.cpp
        ucnv_cb.cpp
        ucnv_cnv.cpp
        ucnv_ct.cpp
        ucnv_err.cpp
        ucnv_ext.cpp
        ucnv_io.cpp
        ucnv_lmb.cpp
        ucnv_set.cpp
        ucnv_u16.cpp
        ucnv_u32.cpp
        ucnv_u7.cpp
        ucnv_u8.cpp
        ucnvbocu.cpp
        ucnvdisp.cpp
        ucnvhz.cpp
        ucnvisci.cpp
        ucnvlat1.cpp
        ucnvmbcs.cpp
        ucnvscsu.cpp
        ucnvsel.cpp
        ucol_swp.cpp
        ucptrie.cpp
        ucurr.cpp
        udata.cpp
        udatamem.cpp
        udataswp.cpp
        uenum.cpp
        uhash.cpp
        uhash_us.cpp
        uidna.cpp
        uinit.cpp
        uinvchar.cpp
        uiter.cpp
        ulist.cpp
        uloc.cpp
        uloc_keytype.cpp
        uloc_tag.cpp
        ulocale.cpp
        ulocbuilder.cpp
        ulocdata.cpp
        umapfile.cpp
        umath.cpp
        umutablecptrie.cpp
        umutex.cpp
        unames.cpp
        unifiedcache.cpp
        unifilt.cpp
        unifunct.cpp
        uniset.cpp
        uniset_closure.cpp
        uniset_props.cpp
        unisetspan.cpp
        unistr.cpp
        unistr_case.cpp
        unistr_case_locale.cpp
        unistr_cnv.cpp
        unistr_props.cpp
        unistr_titlecase_brkiter.cpp
        unorm.cpp
        unormcmp.cpp
        uobject.cpp
        uprops.cpp
        urbtok.cpp
        ures_cnv.cpp
        uresbund.cpp
        uresdata.cpp
        usc_impl.cpp
        uscript.cpp
        uscript_props.cpp
        uset.cpp
        uset_props.cpp
        usetiter.cpp
        ushape.cpp
        usprep.cpp
        ustack.cpp
        ustr_cnv.cpp
        ustr_titlecase_brkiter.cpp
        ustr_wcs.cpp
        ustrcase.cpp
        ustrcase_locale.cpp
        ustrenum.cpp
        ustrfmt.cpp
        ustring.cpp
        ustrtrns.cpp
        utext.cpp
        utf_impl.cpp
        util.cpp
        util_props.cpp
        utrace.cpp
        utrie.cpp
        utrie2.cpp
        utrie2_builder.cpp
        utrie_swap.cpp
        uts46.cpp
        utypes.cpp
        uvector.cpp
        uvectr32.cpp
        uvectr64.cpp
        wintz.cpp)

# ============================================================================
# freebsd-launchd-mach patch (2026-05-15): incbin-based ICU data wiring
# ============================================================================
#
# Replaces upstream's icu_packaged_data.cpp (a giant uint8_t[] = { ...40M
# hex literals... } that peaks at 6-10 GB resident in clang's parser).
#
# Pipeline:
#   1. Build freebsd_decode_icu_chunks (tiny C executable). It reads the
#      4 hex .inc.h files and writes a 32 MB binary blob.
#   2. Run the decoder at build time -> ${CMAKE_CURRENT_BINARY_DIR}/icu_packaged_main_data.bin
#   3. configure_file the .S template, substituting the .bin path.
#   4. Add the generated .S to _FoundationICU's sources. The assembler's
#      .incbin reads the .bin directly into the output .o data section.
#      Compile peak: ~150 MB.
#
# Why: the original .cpp build is structurally infeasible on the project's
# 8 GB CI VM (build-on-low-end-systems rule pinned at .github/workflows/
# build.yml mem: 8192). Even -j 1 OOM-killed at clang's parse phase --
# the 200 MB source TU's AST exceeds 8 GB regardless of optimization
# level or concurrency.
#
# Portability: only target-agnostic GAS pseudo-ops are used in the .S
# template, so it works on any ELF target (x86_64, aarch64, riscv64,
# etc.). Not portable to Mach-O (macOS) or COFF (Windows), but those
# aren't in scope.

# Step 1: build the chunk decoder host-side helper
add_executable(freebsd_decode_icu_chunks freebsd_decode_icu_chunks.c)

# Step 2: emit the binary blob from the 4 hex chunks
set(_freebsd_icu_packaged_bin "${CMAKE_CURRENT_BINARY_DIR}/icu_packaged_main_data.bin")
add_custom_command(
    OUTPUT "${_freebsd_icu_packaged_bin}"
    COMMAND $<TARGET_FILE:freebsd_decode_icu_chunks>
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.0.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.1.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.2.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.3.inc.h"
            "${_freebsd_icu_packaged_bin}"
    DEPENDS freebsd_decode_icu_chunks
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.0.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.1.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.2.inc.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/icu_packaged_main_data.3.inc.h"
    COMMENT "Decoding ICU hex chunks to binary blob (replaces icu_packaged_data.cpp)"
    VERBATIM)

# Step 3: substitute the .bin path AND the version-suffixed data
# symbol name into the .S template. U_ICUDATA_ENTRY_POINT is a
# preprocessor macro that expands to icudt<MAJOR>_dat. We extract
# U_ICU_VERSION_MAJOR_NUM from uvernum.h so the symbol stays in
# sync with the vendored ICU version after upgrades.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../include/_foundation_unicode/uvernum.h"
    _freebsd_uvernum_content)
string(REGEX MATCH "#define[ \t]+U_ICU_VERSION_MAJOR_NUM[ \t]+([0-9]+)"
    _freebsd_uvernum_match "${_freebsd_uvernum_content}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "freebsd-launchd-mach: failed to extract U_ICU_VERSION_MAJOR_NUM from uvernum.h")
endif()
set(ICU_DATA_SYMBOL "icudt${CMAKE_MATCH_1}_dat")
message(STATUS "freebsd-launchd-mach: ICU data symbol = ${ICU_DATA_SYMBOL}")

set(ICU_PACKAGED_BIN_PATH "${_freebsd_icu_packaged_bin}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/freebsd_icu_packaged_data.c.in"
    "${CMAKE_CURRENT_BINARY_DIR}/freebsd_icu_packaged_data.c"
    @ONLY)

# Step 4: add generated .c (with inline asm) to the library + tell
# CMake it depends on the .bin so ninja rebuilds when the .bin
# regenerates.
#
# We use a .c file with a top-level __asm__ block rather than a
# standalone .S because CMake's per-file LANGUAGE override on a .S
# in a child directory doesn't always reach the target's compile
# list (iteration #10 silently dropped the .S from the build). A
# .c file is auto-detected, no override needed.
set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/freebsd_icu_packaged_data.c"
    PROPERTIES
        OBJECT_DEPENDS "${_freebsd_icu_packaged_bin}"
        GENERATED TRUE)
target_sources(_FoundationICU PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/freebsd_icu_packaged_data.c")

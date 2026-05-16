##===----------------------------------------------------------------------===##
##
## This source file is part of the Swift open source project
##
## Copyright (c) 2024 Apple Inc. and the Swift project authors
## Licensed under Apache License v2.0
##
## See LICENSE.txt for license information
## See CONTRIBUTORS.md for the list of Swift project authors
##
## SPDX-License-Identifier: Apache-2.0
##
##===----------------------------------------------------------------------===##

cmake_minimum_required(VERSION 3.24)

# freebsd-launchd-mach patch (2026-05-15): drop Swift from LANGUAGES,
# add C. Upstream targets the swift-foundation package and only needs
# CXX + Swift; we don't have Swift on the FreeBSD chroot, and we add
# C so the freebsd_icu_packaged_data.S incbin replacement (in
# icuSources/common/CMakeLists.txt) can be processed by the C
# compiler driver (clang -c file.S -> preprocess + assemble). The
# decoder helper executable freebsd_decode_icu_chunks is also C.
project(SwiftFoundationICU
    LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "build shared libraries" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckLinkerFlag)
check_linker_flag(CXX "LINKER:--build-id=sha1" LINKER_SUPPORTS_BUILD_ID)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_Swift_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/swift)

# Build flags
add_compile_definitions(
    $<$<COMPILE_LANGUAGE:C,CXX>:U_ATTRIBUTE_DEPRECATED=>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_SHOW_CPLUSPLUS_API=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_SHOW_INTERNAL_API=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_TIMEZONE_PACKAGE="icutz44l">
    $<$<COMPILE_LANGUAGE:C,CXX>:FORTIFY_SOURCE=2>
    $<$<COMPILE_LANGUAGE:C,CXX>:STD_INSPIRED>
    $<$<COMPILE_LANGUAGE:C,CXX>:MAC_OS_X_VERSION_MIN_REQUIRED=101500>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_STRTOD_L=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_XLOCALE_H=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_STRING_VIEW=1>
    # freebsd-launchd-mach patch (2026-05-15): force U_DISABLE_RENAMING=1
    # unconditionally. Upstream gates this on Darwin only, leaving FreeBSD
    # at 0 - which makes ICU rename every public symbol (e.g. ucol_open
    # becomes icu_74_ucol_open via urename.h #defines). CoreFoundation
    # references the un-renamed names directly; without this flag CF
    # cannot link against libicucore.so.
    $<$<COMPILE_LANGUAGE:C,CXX>:U_DISABLE_RENAMING=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_COMBINED_IMPLEMENTATION>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_COMMON_IMPLEMENTATION>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_I18N_IMPLEMENTATION>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_IO_IMPLEMENTATION>
    $<$<COMPILE_LANGUAGE:C,CXX>:ICU_DATA_DIR="/usr/share/icu">
    $<$<COMPILE_LANGUAGE:C,CXX>:USE_PACKAGE_DATA=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:APPLE_ICU_CHANGES=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:UCHAR_TYPE=char16_t>
    $<$<COMPILE_LANGUAGE:C,CXX>:U_PLATFORM_IS_DARWIN_BASED=0>)

# Linux specific settings
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    # Pass -fno-omit-frame-pointer while compiling for better backtraces
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-Xcc -fno-omit-frame-pointer>"
        "$<$<COMPILE_LANGUAGE:C,CXX>:-fno-omit-frame-pointer>")
endif()

# Window specific settings
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_compile_definitions(
        $<$<COMPILE_LANGUAGE:C,CXX>:U_TIMEZONE=_timezone>
        $<$<COMPILE_LANGUAGE:C,CXX>:_CRT_SECURE_NO_DEPRECATE>
        $<$<COMPILE_LANGUAGE:C,CXX>:U_PLATFORM_USES_ONLY_WIN32_API>)
elseif(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    # WASI specific settings
    add_compile_definitions(
        $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_TZSET=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_TZNAME=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:U_HAVE_TIMEZONE=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:HAVE_DLFCN_H=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:HAVE_DLOPEN=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:U_ENABLE_DYLOAD=0>
        $<$<COMPILE_LANGUAGE:C,CXX>:_WASI_EMULATED_SIGNAL>
        $<$<COMPILE_LANGUAGE:C,CXX>:_WASI_EMULATED_MMAN>)
    add_link_options("-Lwasi-emulated-signal")
    add_link_options("-Lwasi-emulated-mman")
elseif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    # On FreeBSD, we should not set U_TIMEZONE
else()
    add_compile_definitions(
        $<$<COMPILE_LANGUAGE:C,CXX>:U_TIMEZONE=timezone>)
endif()

if(BUILD_SHARED_LIBS)
    set(install_directory swift)
else()
    set(install_directory swift_static)
endif()

if(NOT SWIFT_SYSTEM_NAME)
  if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(SWIFT_SYSTEM_NAME macosx)
  else()
    set(SWIFT_SYSTEM_NAME "$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>")
  endif()
endif()

add_subdirectory(icuSources)
add_subdirectory(cmake/modules)

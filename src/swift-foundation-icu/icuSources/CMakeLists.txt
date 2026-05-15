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

add_library(_FoundationICU)

target_include_directories(_FoundationICU
    PUBLIC
        include/)

target_compile_options(_FoundationICU INTERFACE
  "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:Swift>:SHELL:-Xcc -fmodule-map-file=${CMAKE_CURRENT_SOURCE_DIR}/include/_foundation_unicode/module.modulemap>>")

add_subdirectory(common)
add_subdirectory(i18n)
add_subdirectory(io)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_compile_definitions(_FoundationICU PRIVATE SWIFT_PACKAGE=1)
endif()
set_target_properties(_FoundationICU PROPERTIES
    INSTALL_RPATH "$ORIGIN")

if(LINKER_SUPPORTS_BUILD_ID)
  target_link_options(_FoundationICU PRIVATE "LINKER:--build-id=sha1")
endif()

if(ANDROID)
  target_link_options(_FoundationICU PRIVATE "LINKER:-z,max-page-size=16384")
endif()

# Copy Headers to known directory for direct client (XCTest) test builds
file(COPY
        include/
    DESTINATION
        ${CMAKE_BINARY_DIR}/_CModulesForClients)

# Install binary
install(TARGETS _FoundationICU
    ARCHIVE DESTINATION lib/${install_directory}/${SWIFT_SYSTEM_NAME}
    LIBRARY DESTINATION lib/${install_directory}/${SWIFT_SYSTEM_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers
install(DIRECTORY
            include/
        DESTINATION
            lib/${install_directory})

set_property(GLOBAL APPEND PROPERTY SWIFT_FOUNDATION_ICU_EXPORTS _FoundationICU)

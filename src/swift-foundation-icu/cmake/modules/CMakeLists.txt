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

set(SWIFT_FOUNDATION_ICU_EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/SwiftFoundationICUExports.cmake)

configure_file(SwiftFoundationICUConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SwiftFoundationICUConfig.cmake)

get_property(SWIFT_FOUNDATION_ICU_EXPORTS GLOBAL PROPERTY SWIFT_FOUNDATION_ICU_EXPORTS)
export(TARGETS ${SWIFT_FOUNDATION_ICU_EXPORTS}
    NAMESPACE SwiftFoundationICU::
    FILE ${SWIFT_FOUNDATION_ICU_EXPORTS_FILE}
    EXPORT_LINK_INTERFACE_LIBRARIES)

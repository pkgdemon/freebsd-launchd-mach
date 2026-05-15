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
        locbund.cpp
        sprintf.cpp
        sscanf.cpp
        ucln_io.cpp
        ufile.cpp
        ufmt_cmn.cpp
        uprintf.cpp
        uprntf_p.cpp
        uscanf.cpp
        uscanf_p.cpp
        ustdio.cpp
        ustream.cpp)

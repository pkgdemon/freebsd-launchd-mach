//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#if USE_PACKAGE_DATA

#include "icu_packaged_main_data.0.inc.h"
#include "icu_packaged_main_data.1.inc.h"
#include "icu_packaged_main_data.2.inc.h"
#include "icu_packaged_main_data.3.inc.h"

#include <stdint.h>
#include <_foundation_unicode/utypes.h>

extern "C" U_EXPORT uint8_t const U_ICUDATA_ENTRY_POINT[] __attribute__ ((aligned (8))) = {
    __ICU_PACKAGED_MAIN_DATA_CHUNK_0,
    __ICU_PACKAGED_MAIN_DATA_CHUNK_1,
    __ICU_PACKAGED_MAIN_DATA_CHUNK_2,
    __ICU_PACKAGED_MAIN_DATA_CHUNK_3,
};

#endif

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

#ifndef ICU_PACKAGED_DATA_H
#define ICU_PACKAGED_DATA_H

#include "icu_packaged_timezone_data.0.inc.h"
#include "icu_packaged_timezone_data.1.inc.h"
#include "icu_packaged_timezone_data.2.inc.h"
#include "icu_packaged_timezone_data.3.inc.h"

uint8_t const _ICUPackagedTimeZoneData[] __attribute__ ((aligned (8))) = {
    __ICU_PACKAGED_TIMEZONE_DATA_CHUNK_0,
    __ICU_PACKAGED_TIMEZONE_DATA_CHUNK_1,
    __ICU_PACKAGED_TIMEZONE_DATA_CHUNK_2,
    __ICU_PACKAGED_TIMEZONE_DATA_CHUNK_3,
};

#endif // ifndef ICU_PACKAGED_DATA_H

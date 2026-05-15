/*
*****************************************************************************************
* Copyright (C) 2010-2011 Apple Inc. All Rights Reserved.
*****************************************************************************************
*/

#include <_foundation_unicode/utypes.h>

#if !UCONFIG_NO_FORMATTING

#include <_foundation_unicode/udateintervalformat.h>
#include <_foundation_unicode/udatintv.h>

U_NAMESPACE_USE


U_CAPI UDateIntervalFormat* U_EXPORT2
udatintv_open(const char*  locale,
              const UChar* skeleton,
              int32_t      skeletonLength,
              UErrorCode*  status)
{
    return udtitvfmt_open(locale, skeleton, skeletonLength, NULL, 0, status);
}


U_CAPI void U_EXPORT2
udatintv_close(UDateIntervalFormat *datintv)
{
    udtitvfmt_close(datintv);
}


U_INTERNAL int32_t U_EXPORT2
udatintv_format(const UDateIntervalFormat* datintv,
                UDate           fromDate,
                UDate           toDate,
                UChar*          result,
                int32_t         resultCapacity,
                UFieldPosition* position,
                UErrorCode*     status)
{
    return udtitvfmt_format(datintv, fromDate, toDate, result, resultCapacity, position, status);
}


#endif /* #if !UCONFIG_NO_FORMATTING */

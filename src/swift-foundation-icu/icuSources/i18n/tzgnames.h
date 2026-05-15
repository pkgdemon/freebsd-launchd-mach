// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
* Copyright (C) 2011-2012, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef __TZGNAMES_H
#define __TZGNAMES_H

/**
 * \file 
 * \brief C API: Time zone generic names classes
 */

#include <_foundation_unicode/utypes.h>

#if !UCONFIG_NO_FORMATTING

#include <_foundation_unicode/locid.h>
#include <_foundation_unicode/unistr.h>
#include <_foundation_unicode/tzfmt.h>
#include <_foundation_unicode/tznames.h>

U_CDECL_BEGIN

typedef enum UTimeZoneGenericNameType {
    UTZGNM_UNKNOWN      = 0x00,
    UTZGNM_LOCATION     = 0x01,
    UTZGNM_LONG         = 0x02,
    UTZGNM_SHORT        = 0x04
} UTimeZoneGenericNameType;

U_CDECL_END

U_NAMESPACE_BEGIN

class TimeZone;
struct TZGNCoreRef;

class U_I18N_API TimeZoneGenericNames : public UMemory {
public:
    virtual ~TimeZoneGenericNames();

    static TimeZoneGenericNames* createInstance(const Locale& locale, UErrorCode& status);

    virtual bool operator==(const TimeZoneGenericNames& other) const;
    virtual bool operator!=(const TimeZoneGenericNames& other) const {return !operator==(other);}
    virtual TimeZoneGenericNames* clone() const;

    UnicodeString& getDisplayName(const TimeZone& tz, UTimeZoneGenericNameType type,
                        UDate date, UnicodeString& name) const;

    UnicodeString& getGenericLocationName(const UnicodeString& tzCanonicalID, UnicodeString& name) const;

    int32_t findBestMatch(const UnicodeString& text, int32_t start, uint32_t types,
        UnicodeString& tzID, UTimeZoneFormatTimeType& timeType, UErrorCode& status) const;

private:
    TimeZoneGenericNames();
    TZGNCoreRef* fRef;
};

U_NAMESPACE_END
#endif
#endif

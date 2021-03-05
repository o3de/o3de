/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Utility functions to work with GUIDs.


#ifndef CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H
#pragma once

#include "AzCore/Math/Uuid.h"

struct GuidUtil
{
    //! Convert GUID to string in the valid format.
    //! The valid format for a GUID is {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} where X is a hex digit.
    static const char* ToString(REFGUID guid);
    //! Convert from guid string in valid format to GUID class.
    static GUID FromString(const char* guidString);
    static bool IsEmpty(REFGUID guid);

    static const GUID NullGuid;
};

/** Used to compare GUID keys.
*/
struct guid_less_predicate
{
    bool operator()(REFGUID guid1, REFGUID guid2) const
    {
        return memcmp(&guid1, &guid2, sizeof(GUID)) < 0;
    }
};

//////////////////////////////////////////////////////////////////////////
inline bool GuidUtil::IsEmpty(REFGUID guid)
{
    return guid == NullGuid;
}

//////////////////////////////////////////////////////////////////////////
inline const char* GuidUtil::ToString(REFGUID guid)
{
    static char guidString[64];
    sprintf_s(guidString, "{%.8X-%.4X-%.4X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return guidString;
}

//////////////////////////////////////////////////////////////////////////
inline GUID GuidUtil::FromString(const char* guidString)
{
    GUID guid;
    unsigned int d[8];
    memset(&d, 0, sizeof(guid));
    guid.Data1 = 0;
    guid.Data2 = 0;
    guid.Data3 = 0;
    azsscanf(guidString, "{%8" SCNx32 "-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X}",
        &guid.Data1, &guid.Data2, &guid.Data3, &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7]);
    guid.Data4[0] = d[0];
    guid.Data4[1] = d[1];
    guid.Data4[2] = d[2];
    guid.Data4[3] = d[3];
    guid.Data4[4] = d[4];
    guid.Data4[5] = d[5];
    guid.Data4[6] = d[6];
    guid.Data4[7] = d[7];

    return guid;
}

#endif // CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Utility functions to work with GUIDs.


#ifndef CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H
#pragma once

#include <AzCore/Math/Guid.h>

#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
typedef const GUID& REFGUID;
#endif

struct GuidUtil
{
    //! Convert GUID to string in the valid format.
    //! The valid format for a GUID is {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} where X is a hex digit.
    static const char* ToString(REFGUID guid);
    //! Convert from guid string in valid format to GUID class.
    static GUID FromString(const char* guidString);
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

#endif // CRYINCLUDE_EDITOR_UTIL_GUIDUTIL_H

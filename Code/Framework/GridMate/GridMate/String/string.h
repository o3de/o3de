/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CONTAINERS_STRING_H
#define GM_CONTAINERS_STRING_H

#include <GridMate/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>  // for getting from string->wstring and backwards

namespace GridMate
{
    /**
     * All libs should use specialized allocators
     */
    typedef AZStd::basic_string<char, AZStd::char_traits<char>, SysContAlloc > string;
    typedef AZStd::basic_string<wchar_t, AZStd::char_traits<wchar_t>, SysContAlloc > wstring;

    typedef AZStd::basic_string<char, AZStd::char_traits<char>, GridMateStdAlloc > gridmate_string;
    typedef AZStd::basic_string<wchar_t, AZStd::char_traits<wchar_t>, GridMateStdAlloc > gridmate_wstring;
}
#endif // GM_CONTAINERS_STRING_H

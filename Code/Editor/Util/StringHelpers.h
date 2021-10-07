/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H
#pragma once

#include <AzCore/std/string/string.h>
#include <vector>

namespace StringHelpers
{
    // compares two strings to see if they are the same or different, case is ignored
    // returns 0 if the strings are the same, a -1 if the first string is bigger, or a 1 if the second string is bigger
    int CompareIgnoreCase(const AZStd::string& str0, const AZStd::string& str1);
    int CompareIgnoreCase(const AZStd::wstring& str0, const AZStd::wstring& str1);
  
    void Split(const AZStd::string& str, const AZStd::string& separator, bool bReturnEmptyPartsToo, std::vector<AZStd::string>& outParts);
    void Split(const AZStd::wstring& str, const AZStd::wstring& separator, bool bReturnEmptyPartsToo, std::vector<AZStd::wstring>& outParts);

}
#endif // CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H

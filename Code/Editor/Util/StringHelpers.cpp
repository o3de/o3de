/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "StringHelpers.h"
#include "Util.h"
#include <cwctype>

int StringHelpers::CompareIgnoreCase(const AZStd::string& str0, const AZStd::string& str1)
{
    const size_t minLength = Util::getMin(str0.length(), str1.length());
    const int result = azstrnicmp(str0.c_str(), str1.c_str(), minLength);
    if (result)
    {
        return result;
    }
    else
    {
        return (str0.length() == str1.length())
               ? 0
               : ((str0.length() < str1.length()) ? -1 : +1);
    }
}

int StringHelpers::CompareIgnoreCase(const AZStd::wstring& str0, const AZStd::wstring& str1)
{
    const size_t minLength = Util::getMin(str0.length(), str1.length());
    for (size_t i = 0; i < minLength; ++i)
    {
        const wchar_t c0 = towlower(str0[i]);
        const wchar_t c1 = towlower(str1[i]);
        if (c0 != c1)
        {
            return (c0 < c1) ? -1 : 1;
        }
    }

    return (str0.length() == str1.length())
           ? 0
           : ((str0.length() < str1.length()) ? -1 : +1);
}

template <class TS>
static inline void Split_Tpl(const TS& str, const TS& separator, bool bReturnEmptyPartsToo, std::vector<TS>& outParts)
{
    if (str.empty())
    {
        return;
    }

    if (separator.empty())
    {
        for (size_t i = 0; i < str.length(); ++i)
        {
            outParts.push_back(str.substr(i, 1));
        }
        return;
    }

    size_t partStart = 0;

    for (;; )
    {
        const size_t pos = str.find(separator, partStart);
        if (pos == TS::npos)
        {
            break;
        }
        if (bReturnEmptyPartsToo || (pos > partStart))
        {
            outParts.push_back(str.substr(partStart, pos - partStart));
        }
        partStart = pos + separator.length();
    }

    if (bReturnEmptyPartsToo || (partStart < str.length()))
    {
        outParts.push_back(str.substr(partStart, str.length() - partStart));
    }
}

void StringHelpers::Split(const AZStd::string& str, const AZStd::string& separator, bool bReturnEmptyPartsToo, std::vector<AZStd::string>& outParts)
{
    Split_Tpl(str, separator, bReturnEmptyPartsToo, outParts);
}

void StringHelpers::Split(const AZStd::wstring& str, const AZStd::wstring& separator, bool bReturnEmptyPartsToo, std::vector<AZStd::wstring>& outParts)
{
    Split_Tpl(str, separator, bReturnEmptyPartsToo, outParts);
}

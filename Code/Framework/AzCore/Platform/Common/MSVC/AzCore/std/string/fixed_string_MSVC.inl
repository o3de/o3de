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
#pragma once

namespace AZStd
{
    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format_arg(const wchar_t* format, va_list argList)
    {
        basic_fixed_string<wchar_t, MaxElementCount, char_traits<wchar_t>> result;
        int len = _vscwprintf(format, argList);
        if (len > 0)
        {
            result.resize(len);
            len = azvsnwprintf(result.data(), result.capacity() + 1, format, argList);
            AZ_Assert(len == static_cast<int>(result.size()), "azvsnwprintf failed!");
        }

        return result;
    }
}

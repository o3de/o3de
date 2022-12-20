/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            result.resize_no_construct(len);
            len = azvsnwprintf(result.data(), result.capacity() + 1, format, argList);
            AZ_Assert(len == static_cast<int>(result.size()), "azvsnwprintf failed!");
        }

        return result;
    }
}

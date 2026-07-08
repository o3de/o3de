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
        va_list argListCopy;
        va_copy(argListCopy, argList);
        const int len = azvscwprintf(format, argListCopy);
        va_end(argListCopy);

        const bool fitsInCapacity = len >= 0 && static_cast<size_t>(len) <= result.capacity();
        AZ_Assert(
            fitsInCapacity,
            "azvsnwprintf failed! The formatted output does not fit in the fixed_string capacity of %zu wide characters.",
            result.capacity());

        if (fitsInCapacity && len > 0)
        {
            result.resize_no_construct(len);
            va_copy(argListCopy, argList);
            [[maybe_unused]] const int wideCharsPrinted = azvsnwprintf(result.data(), result.size() + 1, format, argListCopy);
            va_end(argListCopy);
            AZ_Assert(wideCharsPrinted == len, "azvsnwprintf failed!");
        }

        return result;
    }
}

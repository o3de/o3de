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
        wchar_t buffer[MaxElementCount + 1];
        const int len = azvsnwprintf(buffer, MaxElementCount + 1, format, argList);

        const bool fitsInCapacity = len >= 0 && static_cast<size_t>(len) <= result.capacity();
        AZ_Assert(
            fitsInCapacity,
            "azvsnwprintf failed! The formatted output does not fit in the fixed_string capacity of %zu wide characters.",
            result.capacity());

        if (fitsInCapacity && len > 0)
        {
            result.append(buffer, len);
        }

        return result;
    }
}

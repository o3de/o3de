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
        constexpr int maxBufferLength = 2048;
        wchar_t buffer[maxBufferLength];
        [[maybe_unused]] const int len = azvsnwprintf(buffer, maxBufferLength, format, argList);
        AZ_Assert(len != -1, "azvsnwprintf failed increase the buffer size!");
        result += buffer;

        return result;
    }
}

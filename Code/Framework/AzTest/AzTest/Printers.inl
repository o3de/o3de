/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ostream>
#include <string_view>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZStd
{
    template<class Element, class Traits, class Allocator>
    void PrintTo(const AZStd::basic_string<Element, Traits, Allocator>& value, ::std::ostream* os)
    {
        if constexpr (AZStd::is_same_v<Element, char>)
        {
            *os << value.c_str();
        }
        else
        {
            AZStd::string utf8String;
            AZStd::to_string(utf8String, value);
            *os << ::std::string_view(utf8String.data(), utf8String.size());
        }
    }

    template<class Element, class Traits>
    void PrintTo(const AZStd::basic_string_view<Element, Traits>& value, ::std::ostream* os)
    {
        if constexpr (AZStd::is_same_v<Element, char>)
        {
            *os << ::std::string_view(value.data(), value.size());
        }
        else
        {
            AZStd::string utf8String;
            AZStd::to_string(utf8String, value);
            *os << ::std::string_view(utf8String.data(), utf8String.size());
        }
    }

    template <class Element, size_t MaxElementCount, class Traits>
    void PrintTo(const AZStd::basic_fixed_string<Element, MaxElementCount, Traits>& value, ::std::ostream* os)
    {
        if constexpr (AZStd::is_same_v<Element, char>)
        {
            *os << value.c_str();
        }
        else
        {
            AZStd::fixed_string<MaxElementCount> utf8String;
            AZStd::to_string(utf8String, value);
            *os << ::std::string_view(utf8String.data(), utf8String.size());
        }
    }
}

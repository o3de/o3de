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
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZStd
{
    template<class Element, class Traits, class Allocator>
    void PrintTo(const AZStd::basic_string<Element, Traits, Allocator>& value, ::std::ostream* os)
    {
        *os << value.c_str();
    }

    template<class Element, class Traits>
    void PrintTo(const AZStd::basic_string_view<Element, Traits>& value, ::std::ostream* os)
    {
        *os << ::std::string_view(value.data(), value.size());
    }

    template <class Element, size_t MaxElementCount, class Traits>
    void PrintTo(const AZStd::basic_fixed_string<Element, MaxElementCount, Traits>& value, ::std::ostream* os)
    {
        *os << value.c_str();
    }
}

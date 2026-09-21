/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <cassert>
#include <cctype>
#include <cfloat>

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/optional.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stack>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

template <typename C, typename... T>
using is_invocable = AZStd::is_invocable<C, T...>;

template <typename C, typename... T>
using invoke_result_t = AZStd::invoke_result_t<C, T...>;

namespace StdUtils = AZStd;
inline constexpr auto none = AZStd::nullopt;

namespace AZ
{
    using AZStd::get;
    using AZStd::holds_alternative;
    using AZStd::monostate;
    using AZStd::optional;
    using AZStd::variant;

    // Configure basic symbols so we can use them unqualified -> easy to change to AzStd without big refactorings.

    using std::enable_if_t;
    using std::is_same_v;

    using std::count;
    using std::exception;
    using std::pair;
    using std::tuple;

    using std::string;
    using std::string_view;

    using std::array;
    using std::map;
    using std::set;
    using std::stack;
    using std::unordered_map;
    using std::unordered_set;
    using std::vector;

    template <typename SetType>
    void SetMerge(SetType& dest, SetType& src)
    {
#ifdef __APPLE__
        for (auto it = src.begin(); it != src.end(); )
        {
            if (dest.find(*it) == dest.end())
            {
                dest.insert(std::move(*it));
                it = src.erase(it);
            }
            else
            {
                ++it;
            }
        }
#else
        dest.merge(src); // when you have correct libraries.
#endif
    }
}

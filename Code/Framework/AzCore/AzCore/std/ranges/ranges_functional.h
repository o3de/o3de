/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges.h>

namespace AZStd::ranges
{
    struct equal_to
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<equality_comparable_with<T, U>, decltype(AZStd::forward<T>(t) == AZStd::forward<U>(u))>
        {
            return AZStd::forward<T>(t) == AZStd::forward<U>(u);
        }
        using is_transparent = void;
    };

    struct not_equal_to
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<equality_comparable_with<T, U>, decltype(!ranges::equal_to{}(AZStd::forward<T>(t), AZStd::forward<U>(u)))>
        {
            return !ranges::equal_to{}(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }
        using is_transparent = void;
    };
    
    struct less
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<totally_ordered_with<T, U>, decltype(AZStd::forward<T>(t) < AZStd::forward<U>(u))>
        {
            return AZStd::forward<T>(t) < AZStd::forward<U>(u);
        }
        using is_transparent = void;
    };

    struct greater
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<totally_ordered_with<T, U>, decltype(ranges::less{}(AZStd::forward<T>(t) < AZStd::forward<U>(u)))>
        {
            return ranges::less{}(AZStd::forward<U>(u) < AZStd::forward<T>(t));
        }
        using is_transparent = void;
    };
    
    struct greater_equal
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<totally_ordered_with<T, U>, decltype(!ranges::less{}(AZStd::forward<T>(t), AZStd::forward<U>(u)))>
        {
            return !ranges::less{}(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }
        using is_transparent = void;
    };
    
    struct less_equal
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const ->
            enable_if_t<totally_ordered_with<T, U>, decltype(!ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t)))>
        {
            return !ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t));
        }
        using is_transparent = void;
    };
} // namespace AZStd::ranges

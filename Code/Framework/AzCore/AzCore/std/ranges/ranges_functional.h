/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/function/identity.h>
#include <AzCore/std/ranges/ranges.h>
#include <iterator>

namespace AZStd::ranges
{
    struct equal_to
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(AZStd::forward<T>(t) == AZStd::forward<U>(u))
            requires equality_comparable_with<T, U>
        {
            return AZStd::forward<T>(t) == AZStd::forward<U>(u);
        }
        using is_transparent = void;
    };

    struct not_equal_to
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(!ranges::equal_to{}(AZStd::forward<T>(t), AZStd::forward<U>(u)))
            requires equality_comparable_with<T, U>
        {
            return !ranges::equal_to{}(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }
        using is_transparent = void;
    };

    struct less
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(AZStd::forward<T>(t) < AZStd::forward<U>(u))
            requires totally_ordered_with<T, U>
        {
            return AZStd::forward<T>(t) < AZStd::forward<U>(u);
        }
        using is_transparent = void;
    };

    struct greater
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t)))
            requires totally_ordered_with<T, U>
        {
            return ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t));
        }
        using is_transparent = void;
    };

    struct greater_equal
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(!ranges::less{}(AZStd::forward<T>(t), AZStd::forward<U>(u)))
            requires totally_ordered_with<T, U>
        {
            return !ranges::less{}(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }
        using is_transparent = void;
    };

    struct less_equal
    {
        template<class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            -> decltype(!ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t)))
            requires totally_ordered_with<T, U>
        {
            return !ranges::less{}(AZStd::forward<U>(u), AZStd::forward<T>(t));
        }
        using is_transparent = void;
    };
} // namespace AZStd::ranges


namespace AZStd
{
    using std::indirectly_comparable;
    using std::mergeable;
    using std::permutable;
    using std::sortable;
}

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


// Iterator Common algorithm requirement concepts
// https://eel.is/c++draft/alg.req
namespace AZStd
{
    template<class I1, class I2, class R, class P1 = identity, class P2 = identity, class = void>
    /*concept*/ constexpr bool indirectly_comparable = false;

    template<class I1, class I2, class R, class P1, class P2>
    /*concept*/ constexpr bool indirectly_comparable<I1, I2, R, P1, P2, enable_if_t<
        indirect_binary_predicate<R, projected<I1, P1>, projected<I2, P2>>>> = true;

    template<class I, class = void>
    /*concept*/ constexpr bool permutable = false;

    template<class I>
    /*concept*/ constexpr bool permutable<I, enable_if_t<conjunction_v<
        bool_constant<forward_iterator<I>>,
        bool_constant<indirectly_movable_storable<I, I>>,
        bool_constant<indirectly_swappable<I, I>> >>> = true;

    template<class I1, class I2, class Out, class R = ranges::less, class P1 = identity, class P2 = identity, class = void>
    /*concept*/ constexpr bool mergeable = false;

    template<class I1, class I2, class Out, class R, class P1, class P2>
    /*concept*/ constexpr bool mergeable<I1, I2, Out, R, P1, P2, enable_if_t<conjunction_v<
        bool_constant<input_iterator<I1>>,
        bool_constant<input_iterator<I2>>,
        bool_constant<weakly_incrementable<Out>>,
        bool_constant<indirectly_copyable<I1, Out>>,
        bool_constant<indirectly_copyable<I2, Out>>,
        bool_constant<indirect_strict_weak_order<R, projected<I1, P1>, projected<I2, P2>>> >>> = true;

    template<class I, class R = ranges::less, class P = identity, class = void>
    /*concept*/ constexpr bool sortable = false;
    template<class I, class R, class P>
    /*concept*/ constexpr bool sortable<I, R, P, enable_if_t<conjunction_v<
        bool_constant<permutable<I>>,
        bool_constant<indirect_strict_weak_order<R, projected<I, P>>> >>> = true;
}

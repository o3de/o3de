/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>

#include <AzCore/std/concepts/concepts_assignable.h>
#include <AzCore/std/concepts/concepts_constructible.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/move.h>
#include <AzCore/std/utility/declval.h>


namespace AZStd
{
    // Bring std utility functions into AZStd namespace
    using std::forward;
}

namespace AZStd::ranges::Internal
{
    template <class T>
    void swap(T&, T&) = delete;

    template <class T, class U, class = void>
    constexpr bool is_class_or_enum_with_swap_adl = false;
    template <class T, class U>
    constexpr bool is_class_or_enum_with_swap_adl<T, U, enable_if_t<conjunction_v<
        disjunction<
            disjunction<is_class<remove_cvref_t<T>>, is_enum<remove_cvref_t<T>>>,
            disjunction<is_class<remove_cvref_t<U>>, is_enum<remove_cvref_t<U>>>>,
        is_void<void_t<decltype(swap(declval<T&>(), declval<U&>()))>>
        >>> = true;

    struct swap_fn
    {
        template <class T, class U>
        constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(swap(AZStd::forward<T>(t), AZStd::forward<U>(u))))
            ->enable_if_t<is_class_or_enum_with_swap_adl<T, U>>
        {
            swap(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }

        // ranges::swap customization point https://eel.is/c++draft/concepts#concept.swappable-2.2
        // Implemented in ranges.h as to prevent circular dependency.
        // ranges::swap_ranges depends on the range concepts that can't be defined here
        template <class T, class U>
        constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept((*this)(*t, *u)))
            ->enable_if_t<conjunction_v<
            bool_constant<!is_class_or_enum_with_swap_adl<T, U>>,
            is_array<T>,
            is_array<U>,
            bool_constant<extent_v<T> == extent_v<U>>
            >>;

        template <class T>
        constexpr auto operator()(T& t1, T& t2) const noexcept(noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_move_assignable_v<T>))
            ->enable_if_t<conjunction_v<bool_constant<!is_class_or_enum_with_swap_adl<T, T>>,
                bool_constant<move_constructible<T>>,
                bool_constant<assignable_from<T&, T>> >>
        {
            auto temp(AZStd::move(t1));
            t1 = AZStd::move(t2);
            t2 = AZStd::move(temp);
        }
    };
}

namespace AZStd::ranges
{
    inline namespace customization_point_object
    {
        inline constexpr auto swap = Internal::swap_fn{};
    }
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool swappable_impl = false;
    template <class T>
    constexpr bool swappable_impl<T, void_t<decltype(AZStd::ranges::swap(declval<T&>(), declval<T&>()))>> = true;

    template <class T, class U, class = void>
    constexpr bool swappable_with_impl = false;
    template <class T, class U>
    constexpr bool swappable_with_impl<T, U, enable_if_t<conjunction_v<
        bool_constant<common_reference_with<T, U>>,
        is_void<void_t<
        decltype(AZStd::ranges::swap(declval<T&>(), declval<T&>())),
        decltype(AZStd::ranges::swap(declval<U&>(), declval<U&>())),
        decltype(AZStd::ranges::swap(declval<T&>(), declval<U&>())),
        decltype(AZStd::ranges::swap(declval<U&>(), declval<T&>()))>> >>> = true;
}

namespace AZStd
{
    template<class T>
    /*concept*/ constexpr bool swappable = Internal::swappable_impl<T>;

    template<class T, class U>
    /*concept*/ constexpr bool swappable_with = Internal::swappable_with_impl<T, U>;
}

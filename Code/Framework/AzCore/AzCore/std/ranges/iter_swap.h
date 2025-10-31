/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>

#include <AzCore/std/iterator/iterator_primitives.h>
#include <AzCore/std/ranges/swap.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>


namespace AZStd::ranges::Internal
{
    template<class I1, class I2>
    void iter_swap(I1, I2) = delete;

    template <class I1, class I2, class = void>
    constexpr bool is_class_or_enum_with_iter_swap_adl = false;

    template <class I1, class I2>
    constexpr bool is_class_or_enum_with_iter_swap_adl<I1, I2, enable_if_t<conjunction_v<
        disjunction<
        disjunction<is_class<remove_cvref_t<I1>>, is_enum<remove_cvref_t<I1>>>,
        disjunction<is_class<remove_cvref_t<I2>>, is_enum<remove_cvref_t<I2>>>>,
        is_void<void_t<decltype(iter_swap(declval<I1>(), declval<I2>()))>>
        >>> = true;

    struct iter_swap_fn
    {
        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<is_class_or_enum_with_iter_swap_adl<I1, I2>
            >
        {
            iter_swap(AZStd::forward<I1>(i1), AZStd::forward<I1>(i2));
        }
        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<conjunction_v<bool_constant<!is_class_or_enum_with_iter_swap_adl<I1, I2>>,
            bool_constant<indirectly_readable<I1>>,
            bool_constant<indirectly_readable<I2>>,
            bool_constant<swappable_with<iter_reference_t<I1>, iter_reference_t<I2>>>
            >>
        {
            ranges::swap(*i1, *i2);
        }

        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<conjunction_v<bool_constant<!is_class_or_enum_with_iter_swap_adl<I1, I2>>,
            bool_constant<!swappable_with<iter_reference_t<I1>, iter_reference_t<I2>>>,
            bool_constant<indirectly_movable_storable<I1, I2>>,
            bool_constant<indirectly_movable_storable<I2, I1>>
            >>
        {
            *AZStd::forward<I1>(i1) = iter_exchange_move(AZStd::forward<I2>(i2), AZStd::forward<I1>(i1));
        }

    private:
        template<class X, class Y>
        static constexpr iter_value_t<X> iter_exchange_move(X&& x, Y&& y)
            noexcept(noexcept(iter_value_t<X>(iter_move(x))) && noexcept(*x = iter_move(y)))
        {
            iter_value_t<X> old_value(iter_move(x));
            *x = iter_move(y);
            return old_value;
        }
    };
}

namespace AZStd::ranges
{
    // Workaround for clang bug https://bugs.llvm.org/show_bug.cgi?id=37556
    // Using a placeholder namespace to wrap the customization point object
    // inline namespce and then bring that placeholder namespace into scope
    namespace workaround
    {
        inline namespace customization_point_object
        {
            inline constexpr Internal::iter_swap_fn iter_swap{};
        }
    }
}

namespace AZStd::Internal
{
    template <class I1, class I2, class = void>
    constexpr bool indirectly_swappable_impl = false;
    template <class I1, class I2>
    constexpr bool indirectly_swappable_impl<I1, I2, enable_if_t<conjunction_v<
        bool_constant<indirectly_readable<I1>>,
        bool_constant<indirectly_readable<I2>>,
        is_void<void_t<
        decltype(AZStd::ranges::iter_swap(declval<I1>(), declval<I1>())),
        decltype(AZStd::ranges::iter_swap(declval<I2>(), declval<I2>())),
        decltype(AZStd::ranges::iter_swap(declval<I1>(), declval<I2>())),
        decltype(AZStd::ranges::iter_swap(declval<I2>(), declval<I1>()))>>
        >>> = true;
}

namespace AZStd
{
    template<class I1, class I2 = I1>
    /*concept*/ constexpr bool indirectly_swappable = Internal::indirectly_swappable_impl<I1, I2>;
}


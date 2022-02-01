/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>

#include <AzCore/std/typetraits/conditional.h>

#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/move.h>
#include <AzCore/std/utility/declval.h>


namespace AZStd
{
    // Bring in std utility functions into AZStd namespace
    using std::forward;
}

// C++20 range traits for iteratable types
namespace AZStd::ranges::Internal
{
    void iter_move();

    template <typename It, typename = void>
    constexpr bool iter_move_adl = false;

    template <typename It>
    constexpr bool iter_move_adl<It, void_t<decltype(iter_move(declval<It>()))>> = true;

    template <typename It, typename = void>
    constexpr bool is_class_or_enum_with_iter_move_adl = false;

    template <typename It>
    constexpr bool is_class_or_enum_with_iter_move_adl<It, enable_if_t<iter_move_adl<It>
        && (is_class_v<remove_cvref_t<It>> || is_enum_v<remove_cvref_t<It>>)>>
        = true;

    struct iter_move_fn
    {
        template <typename It>
        constexpr auto operator()(It&& it) const
            ->enable_if_t<is_class_or_enum_with_iter_move_adl<It>,
            decltype(iter_move(AZStd::forward<It>(it)))>
        {
            return iter_move(AZStd::forward<It>(it));
        }
        template <typename It>
        constexpr auto operator()(It&& it) const
            ->enable_if_t<!is_class_or_enum_with_iter_move_adl<It>&& is_lvalue_reference_v<decltype(*AZStd::forward<It>(it))>,
            decltype(AZStd::move(*AZStd::forward<It>(it)))>
        {
            return AZStd::move(*AZStd::forward<It>(it));
        }
        template <typename It>
        constexpr auto operator()(It&& it) const
            ->enable_if_t<!is_class_or_enum_with_iter_move_adl<It> && !is_lvalue_reference_v<decltype(*AZStd::forward<It>(it))>,
            decltype(*AZStd::forward<It>(it))>
        {
            return *AZStd::forward<It>(it);
        }
    };
}

namespace AZStd::ranges
{
    inline namespace customization_point_object
    {
        inline constexpr auto iter_move = Internal::iter_move_fn{};
    }
}

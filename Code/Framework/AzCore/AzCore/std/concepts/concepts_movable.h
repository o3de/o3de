/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/concepts/concepts_assignable.h>
#include <AzCore/std/concepts/concepts_constructible.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/ranges/swap.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/is_object.h>

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool movable_impl = false;
    template <class T>
    constexpr bool movable_impl<T, enable_if_t<conjunction_v<
        is_object<T>,
        bool_constant<move_constructible<T>>,
        bool_constant<assignable_from<T&, T>>,
        bool_constant<swappable<T>>
        >>> = true;
}

namespace AZStd
{
    // movable
    template <class T>
    /*concept*/ constexpr bool movable = Internal::movable_impl<T>;
}

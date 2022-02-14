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
#include <AzCore/std/concepts/concepts_movable.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/conjunction.h>

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool copy_constructible_impl = false;
    template <class T>
    constexpr bool copy_constructible_impl<T, enable_if_t<conjunction_v<
        bool_constant<move_constructible<T>>,
        bool_constant<constructible_from<T, T&>>,
        bool_constant<convertible_to<T&, T>>,
        bool_constant<constructible_from<T, const T&>>,
        bool_constant<convertible_to<const T&, T>>,
        bool_constant<constructible_from<T, const T>>,
        bool_constant<convertible_to<const T, T>>
        >>> = true;
}

namespace AZStd
{
    //  copy constructible
    template<class T>
    /*concept*/ constexpr bool copy_constructible = Internal::copy_constructible_impl<T>;
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool copyable_impl = false;
    template <class T>
    constexpr bool copyable_impl<T, enable_if_t<conjunction_v<
        bool_constant<copy_constructible<T>>,
        bool_constant<movable<T>>,
        bool_constant<assignable_from<T&, T&>>,
        bool_constant<assignable_from<T&, const T&>>,
        bool_constant<assignable_from<T&, const T>>
        >>> = true;
}

namespace AZStd
{
    // copyable
    template<class T>
    /*concept*/ constexpr bool copyable = Internal::copyable_impl<T>;

}

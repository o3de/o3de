/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_destructible.h>

namespace AZStd
{

    template<class T, class... Args>
    /*concept*/ constexpr bool constructible_from = conjunction_v<bool_constant<destructible<T>>,
        is_constructible<T, Args...> >;

    template<class T>
    /*concept*/ constexpr bool move_constructible = conjunction_v<bool_constant<constructible_from<T, T>>,
        bool_constant<convertible_to<T, T>>>;
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>

namespace AZStd
{
    using std::is_convertible;
    using std::is_convertible_v;

    // models the C++20 convertible_to concept
    namespace Internal
    {
        template<typename From, typename To, typename = void>
        constexpr bool convertible_to_impl = false;
        template<typename From, typename To>
        constexpr bool convertible_to_impl<From, To, enable_if_t<
            is_convertible_v<From, To>, void_t<decltype(static_cast<To>(declval<From>()))>>> = true;
    }
    template<typename From, typename To>
    /*concept*/ constexpr bool convertible_to = Internal::convertible_to_impl<From, To>;
}


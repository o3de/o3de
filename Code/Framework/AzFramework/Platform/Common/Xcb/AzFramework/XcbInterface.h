/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <xcb/xcb.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzFramework
{
    // @brief Wrap a function pointer in a type
    // This serves as a convenient way to wrap a function pointer in a given
    // type. That type can then be used in a `unique_ptr` or `shared_ptr`.
    // Using a type instead of a function pointer by value prevents the need to
    // copy the pointer when copying the smart poiner.
    template<auto Callable>
    struct XcbDeleterFreeFunctionWrapper
    {
        using value_type = decltype(Callable);
        static constexpr value_type s_value = Callable;
        constexpr operator value_type() const noexcept
        {
            return s_value;
        }
    };

    template<typename T, auto fn>
    using XcbUniquePtr = AZStd::unique_ptr<T, XcbDeleterFreeFunctionWrapper<fn>>;

    template<typename T>
    using XcbStdFreePtr = XcbUniquePtr<T, ::free>;
} // namespace AzFramework

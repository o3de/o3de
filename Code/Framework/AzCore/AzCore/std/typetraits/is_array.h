/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    using std::is_array;
    using std::is_array_v;

    // C++20 is_bounded_array and is_unbounded_array
    template<class T>
    struct is_bounded_array : false_type {};

    template<class T, size_t N>
    struct is_bounded_array<T[N]> : true_type {};

    template<class T>
    constexpr bool is_bounded_array_v = is_bounded_array<T>::value;

    template<class T>
    struct is_unbounded_array : false_type {};

    template<class T>
    struct is_unbounded_array<T[]> : true_type {};

    template<class T>
    constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/utility/move.h>

namespace AZStd
{
    /**
    * Forward declare of a C++20 compliant array class
     */
    template<class T, size_t N>
    class array;

    // implementation of the std::get function within the AZStd::namespace which allows AZStd::apply to be used
    // with AZStd::array
    template<size_t I, class T, size_t N>
    constexpr T& get(array<T, N>& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr const T& get(const array<T, N>& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr T&& get(array<T, N>&& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };

    template<size_t I, class T, size_t N>
    constexpr const T&& get(const array<T, N>&& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };
}

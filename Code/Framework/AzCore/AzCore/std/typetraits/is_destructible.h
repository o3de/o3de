/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <type_traits>

namespace AZStd
{
    using std::is_destructible;
    using std::is_trivially_destructible;
    using std::is_nothrow_destructible;
    template<class T>
    constexpr bool is_default_destructible_v = std::is_destructible<T>::value;
    template<class T>
    constexpr bool is_trivially_destructible_v = std::is_trivially_destructible<T>::value;
    template<class T>
    constexpr bool is_nothrow_destructible_v = std::is_nothrow_destructible<T>::value;
}

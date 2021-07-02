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

    using std::is_assignable;
    using std::is_trivially_assignable;
    using std::is_nothrow_assignable;

    using std::is_copy_assignable;
    using std::is_trivially_copy_assignable;
    using std::is_nothrow_copy_assignable;

    using std::is_move_assignable;
    using std::is_trivially_move_assignable;
    using std::is_nothrow_move_assignable;

    template<class T, class U>
    constexpr bool is_assignable_v = std::is_assignable<T, U>::value;
    template<class T, class U>
    constexpr bool is_trivially_assignable_v = std::is_trivially_assignable<T, U>::value;
    template<class T, class U>
    constexpr bool is_nothrow_assignable_v = std::is_nothrow_assignable<T, U>::value;

    template<class T>
    constexpr bool is_copy_assignable_v = std::is_copy_assignable<T>::value;
    template<class T>
    constexpr bool is_trivially_copy_assignable_v = std::is_trivially_copy_assignable<T>::value;
    template<class T>
    constexpr bool is_nothrow_copy_assignable_v = std::is_nothrow_copy_assignable<T>::value;

    template<class T>
    constexpr bool is_move_assignable_v = std::is_move_assignable<T>::value;
    template<class T>
    constexpr bool is_trivially_move_assignable_v = std::is_trivially_move_assignable<T>::value;
    template<class T>
    constexpr bool is_nothrow_move_assignable_v = std::is_nothrow_move_assignable<T>::value;

}

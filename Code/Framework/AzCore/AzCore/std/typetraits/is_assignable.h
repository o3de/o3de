/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

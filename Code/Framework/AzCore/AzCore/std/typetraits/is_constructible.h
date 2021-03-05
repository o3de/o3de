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
    using std::is_constructible;
    using std::is_trivially_constructible;
    using std::is_nothrow_constructible;

    using std::is_default_constructible;
    using std::is_trivially_default_constructible;
    using std::is_nothrow_default_constructible;

    using std::is_copy_constructible;
    using std::is_trivially_copy_constructible;
    using std::is_nothrow_copy_constructible;

    using std::is_move_constructible;
    using std::is_trivially_move_constructible;
    using std::is_nothrow_move_constructible;

    template<class T, class... Args>
    constexpr bool is_constructible_v = std::is_constructible<T, Args...>::value;

    template<class T, class... Args>
    constexpr bool is_trivially_constructible_v = std::is_trivially_constructible<T, Args...>::value;

    template<class T, class... Args>
    constexpr bool is_nothrow_constructible_v = std::is_nothrow_constructible<T, Args...>::value;

    template<class T>
    constexpr bool is_default_constructible_v = std::is_default_constructible<T>::value;
    template<class T>
    constexpr bool is_trivially_default_constructible_v = std::is_trivially_default_constructible<T>::value;
    template<class T>
    constexpr bool is_nothrow_default_constructible_v = std::is_nothrow_default_constructible<T>::value;

    template<class T>
    constexpr bool is_copy_constructible_v = std::is_copy_constructible<T>::value;
    template<class T>
    constexpr bool is_trivially_copy_constructible_v = std::is_trivially_copy_constructible<T>::value;
    template<class T>
    constexpr bool is_nothrow_copy_constructible_v = std::is_nothrow_copy_constructible<T>::value;

    template<class T>
    constexpr bool is_move_constructible_v = std::is_move_constructible<T>::value;
    template<class T>
    constexpr bool is_trivially_move_constructible_v = std::is_trivially_move_constructible<T>::value;
    template<class T>
    constexpr bool is_nothrow_move_constructible_v = std::is_nothrow_move_constructible<T>::value;

}

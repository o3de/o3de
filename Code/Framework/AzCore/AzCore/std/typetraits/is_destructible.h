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

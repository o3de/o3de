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

#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename ...Ts>
        struct pack_traits_arg_sequence {};

        template <size_t index, typename T>
        struct pack_traits_get_arg;

        template <size_t index>
        struct pack_traits_get_arg<index, pack_traits_arg_sequence<>>
        {
            static_assert(AZStd::integral_constant<size_t, index>::value && false, "Element index out of bounds");
        };

        template <typename First, typename ...Rest>
        struct pack_traits_get_arg<0, pack_traits_arg_sequence<First, Rest...>>
        {
            using type = First;
        };

        template <size_t index, typename First, typename ...Rest>
        struct pack_traits_get_arg<index, pack_traits_arg_sequence<First, Rest...>> : pack_traits_get_arg<index - 1, pack_traits_arg_sequence<Rest...>> {};

        template <size_t index, typename ...Args>
        using pack_traits_get_arg_t = typename pack_traits_get_arg<index, pack_traits_arg_sequence<Args...>>::type;
    }
}
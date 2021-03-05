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

#include <AzCore/std/typetraits/internal/type_sequence_traits.h>
#include <AzCore/std/tuple.h>

namespace AZStd
{
    namespace Internal
    {
        template<typename>
        struct tuple_elements_sequence_helper {};

        template<typename ...t_Elements>
        struct tuple_elements_sequence_helper<std::tuple<t_Elements...>>
        {
            using TupleType = std::tuple<t_Elements...>;
            AZSTD_STATIC_CONSTANT(size_t, size = std::tuple_size<TupleType>::value);

            template<size_t index>
            using get_element_t = Internal::pack_traits_get_arg_t<index, t_Elements...>;
            using elements_sequence = Internal::pack_traits_arg_sequence<t_Elements...>;
        };
    }    

    template<typename Tuple>
    struct tuple_elements_sequence : public Internal::tuple_elements_sequence_helper<Tuple> {};
}

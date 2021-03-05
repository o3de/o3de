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

#include <AzCore/std/typetraits/conditional.h>

namespace AZStd
{
    template<class... Bases>
    struct conjunction
        : true_type
    {
    };

    template<class B1>
    struct conjunction<B1>
        : B1
    {
    };

    template<class B1, class... Bases>
    struct conjunction<B1, Bases...>
        : conditional_t<!B1::value, B1, conjunction<Bases...>>
    {
    };

    template<class... Bases>
    constexpr bool conjunction_v = conjunction<Bases...>::value;
} // namespace AZStd

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/conditional.h>

namespace AZStd
{
    template<class... Bases>
    struct disjunction
        : false_type
    {
    };

    template<class B1>
    struct disjunction<B1>
        : B1
    {
    };

    template<class B1, class... Bases>
    struct disjunction<B1, Bases...>
        : conditional_t<B1::value, B1, disjunction<Bases...>>
    {
    };

    template<class... Bases>
    constexpr bool disjunction_v = disjunction<Bases...>::value;
}

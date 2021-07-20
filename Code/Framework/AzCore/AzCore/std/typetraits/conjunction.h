/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    // Preserve the historical make_void indirection so SFINAE partial specializations
    // remain distinct until substitution occurs.
    template<typename... Args>
    struct make_void
    {
        using type = void;
    };

    template<typename... Args>
    using void_t = typename make_void<Args...>::type;
}

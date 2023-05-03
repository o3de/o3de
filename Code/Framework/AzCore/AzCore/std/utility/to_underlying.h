/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/underlying_type.h>

namespace AZStd
{
    template<class Enum>
    constexpr AZStd::underlying_type_t<Enum> to_underlying(Enum value) noexcept
    {
        return static_cast<AZStd::underlying_type_t<Enum>>(value);
    }
}

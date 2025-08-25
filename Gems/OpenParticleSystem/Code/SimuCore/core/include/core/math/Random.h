/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include <AzCore/Math/Color.h>

namespace SimuCore {
    class Random {
    public:
        static float Rand();
       
        static float RandomRange(float min, float max);

        static uint32_t RandomRange(uint32_t min, uint32_t max);

        static AZ::Color RandomRange(const AZ::Color& min, const AZ::Color& max);
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>

namespace SimuCore {
    class Random {
    public:
        static float Rand();
       
        static float RandomRange(float min, float max);

        static AZ::u32 RandomRange(AZ::u32 min, AZ::u32 max);

        static AZ::Color RandomRange(const AZ::Color& min, const AZ::Color& max);
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>

namespace AZ
{
    namespace Render
    {
        namespace DepthOfField
        {
            // Depth of Field Constants...

            static constexpr uint32_t QualityLevelMax = 2;
            static constexpr float ApertureFMin = 0.12f;
            static constexpr float ApertureFMax = 256.0f;

            // Maximum speed per second of focus.
            // As a standard, the distance from near to far is 1.0.
            static constexpr float AutoFocusSpeedMax = 2.0f;

            // The unit is seconds.
            static constexpr float AutoFocusDelayMax = 1.0f;
        }
    }
}

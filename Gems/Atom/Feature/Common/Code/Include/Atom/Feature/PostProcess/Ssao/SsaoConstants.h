/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        namespace Ssao
        {
            static constexpr float DefaultStrength = 1.0f;
            static constexpr float DefaultSamplingRadius = 0.05f;

            static constexpr float DefaultBlurConstFalloff = 0.85f;
            static constexpr float DefaultBlurDepthFalloffThreshold = 0.0f;
            static constexpr float DefaultBlurDepthFalloffStrength = 200.0f;
        }
    }
}

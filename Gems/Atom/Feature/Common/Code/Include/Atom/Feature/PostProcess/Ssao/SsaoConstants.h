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

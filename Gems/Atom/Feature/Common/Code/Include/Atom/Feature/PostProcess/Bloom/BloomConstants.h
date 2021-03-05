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

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace Render
    {
        namespace Bloom
        {
            static constexpr uint8_t MaxStageCount = 5;

            static constexpr float DefaultThreshold = 1.0f;

            // Length of the soft threshold's tail, larger value leads to soomther
            // transition on the edge, set to 0 to disable
            static constexpr float DefaultKnee = 0.5f;

            static constexpr float DefaultKernelSizeScale = 1.0f;
            static constexpr float DefaultScreenPercentStage0 = 0.04f;
            static constexpr float DefaultScreenPercentStage1 = 0.08f;
            static constexpr float DefaultScreenPercentStage2 = 0.16f;
            static constexpr float DefaultScreenPercentStage3 = 0.32f;
            static constexpr float DefaultScreenPercentStage4 = 0.64f;

            static Vector3 DefaultTint = Vector3(1.0f);

            static constexpr bool DefaultEnableBicubicFilter = false;

            static constexpr float DefaultIntensity = 0.5f;
        }
    }
}

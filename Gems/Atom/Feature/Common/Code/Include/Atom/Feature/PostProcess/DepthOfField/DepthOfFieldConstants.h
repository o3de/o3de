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

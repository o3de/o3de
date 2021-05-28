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
#include <DebugCameraUtils.h>

#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace Debug
    {
        void ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
        {
            float blendedValue;

            blendedValue = AZ::Lerp(newValue, oldValue, deltaTime);

            oldValue = blendedValue;
            newValue = blendedValue;
        }

        float NormalizeAngle(float angle)
        {
            if (angle > AZ::Constants::Pi)
            {
                angle -= AZ::Constants::TwoPi;
            }
            else if (angle < -AZ::Constants::Pi)
            {
                angle += AZ::Constants::TwoPi;
            }
            return angle;
        }
    } // namespace Debug
} // namespace AZ

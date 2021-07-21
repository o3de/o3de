/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

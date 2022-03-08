/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Velocity.h>
#include <AzCore/Math/MathUtils.h>

namespace EMotionFX
{
    AZ::Vector3 CalculateLinearVelocity(const AZ::Vector3& lastPosition,
        const AZ::Vector3& currentPosition,
        float timeDelta)
    {
        if (timeDelta <= AZ::Constants::FloatEpsilon)
        {
            return AZ::Vector3::CreateZero();
        }

        const AZ::Vector3 deltaPosition = currentPosition - lastPosition;
        const AZ::Vector3 velocity = deltaPosition / timeDelta;

        if (velocity.GetLength() > AZ::Constants::FloatEpsilon)
        {
            return velocity;
        }

        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 CalculateAngularVelocity(const AZ::Quaternion& lastRotation,
        const AZ::Quaternion& currentRotation,
        float timeDelta)
    {
        if (timeDelta <= AZ::Constants::FloatEpsilon)
        {
            return AZ::Vector3::CreateZero();
        }

        const AZ::Quaternion deltaRotation = currentRotation * lastRotation.GetInverseFull();
        const AZ::Quaternion shortestEquivalent = deltaRotation.GetShortestEquivalent().GetNormalized();
        const AZ::Vector3 scaledAxisAngle = shortestEquivalent.ConvertToScaledAxisAngle();
        const AZ::Vector3 angularVelocity = scaledAxisAngle / timeDelta;

        if (angularVelocity.GetLength() > AZ::Constants::FloatEpsilon)
        {
            return angularVelocity;
        }

        return AZ::Vector3::CreateZero();
    }

    void DebugDrawVelocity(AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& position, const AZ::Vector3& velocity, const AZ::Color& color)
    {
        // Don't visualize joints that remain motionless (zero velocity).
        if (velocity.GetLength() < AZ::Constants::FloatEpsilon)
        {
            return;
        }

        const float scale = 0.15f;
        const AZ::Vector3 arrowPosition = position + velocity;

        debugDisplay.DepthTestOff();
        debugDisplay.SetColor(color);

        debugDisplay.DrawSolidCylinder(/*center=*/(arrowPosition + position) * 0.5f,
            /*direction=*/(arrowPosition - position).GetNormalizedSafe(),
            /*radius=*/0.003f,
            /*height=*/(arrowPosition - position).GetLength(),
            /*drawShaded=*/false);

        debugDisplay.DrawSolidCone(position + velocity,
            velocity,
            0.1f * scale,
            scale * 0.5f,
            /*drawShaded=*/false);
    }
} // namespace EMotionFX

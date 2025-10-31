/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    AZ::Vector3 EMFX_API CalculateLinearVelocity(const AZ::Vector3& lastPosition, const AZ::Vector3& currentPosition, float timeDelta);
    AZ::Vector3 EMFX_API CalculateAngularVelocity(const AZ::Quaternion& lastRotation, const AZ::Quaternion& currentRotation, float timeDelta);

    void EMFX_API DebugDrawVelocity(AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& position, const AZ::Vector3& velocity, const AZ::Color& color);
} // namespace EMotionFX

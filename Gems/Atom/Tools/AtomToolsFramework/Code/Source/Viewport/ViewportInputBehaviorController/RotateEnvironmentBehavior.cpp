/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>

namespace AtomToolsFramework
{
    RotateEnvironmentBehavior::RotateEnvironmentBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void RotateEnvironmentBehavior::TickInternal(float x, float y, float z)
    {
        ViewportInputBehavior::TickInternal(x, y, z);

        m_rotation += x;

        const auto& entityId = m_controller->GetEnvironmentEntityId();
        const auto& rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), m_rotation);
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldRotationQuaternion, rotation);
    }

    float RotateEnvironmentBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float RotateEnvironmentBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

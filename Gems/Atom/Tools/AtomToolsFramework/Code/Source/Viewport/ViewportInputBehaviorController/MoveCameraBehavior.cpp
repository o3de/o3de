/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/MoveCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector3.h>

namespace AtomToolsFramework
{
    MoveCameraBehavior::MoveCameraBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void MoveCameraBehavior::End()
    {
        float distanceToTarget = m_controller->GetDistanceToTarget();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 targetPosition = transform.GetTranslation() + transform.GetBasisY() * distanceToTarget;
        m_controller->SetTargetPosition(targetPosition);
    }

    void MoveCameraBehavior::TickInternal(float x, float y, float z)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 up = transform.GetBasisZ();
        AZ::Vector3 right = transform.GetBasisX();
        AZ::Vector3 position = transform.GetTranslation();
        AZ::Vector3 deltaPosition = up * y + right * -x;
        position += deltaPosition;
        m_targetPosition += deltaPosition;
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);

        ViewportInputBehavior::TickInternal(x, y, z);
    }

    float MoveCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float MoveCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

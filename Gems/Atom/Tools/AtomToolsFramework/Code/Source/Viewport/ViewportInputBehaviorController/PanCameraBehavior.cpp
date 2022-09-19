/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector3.h>

namespace AtomToolsFramework
{
    PanCameraBehavior::PanCameraBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void PanCameraBehavior::End()
    {
        float objectDistance = m_controller->GetObjectDistance();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 objectPosition = transform.GetTranslation() + transform.GetBasisY() * objectDistance;
        m_controller->SetObjectPosition(objectPosition);
    }

    void PanCameraBehavior::TickInternal(float x, float y, float z)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 up = transform.GetBasisZ();
        AZ::Vector3 right = transform.GetBasisX();
        AZ::Vector3 position = transform.GetTranslation();
        AZ::Vector3 deltaPosition = up * y + right * -x;
        position += deltaPosition;
        m_objectPosition += deltaPosition;
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);

        ViewportInputBehavior::TickInternal(x, y, z);
    }

    float PanCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float PanCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

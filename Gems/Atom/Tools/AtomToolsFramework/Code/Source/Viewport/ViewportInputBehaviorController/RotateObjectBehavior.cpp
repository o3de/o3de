/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>

namespace AtomToolsFramework
{
    RotateObjectBehavior::RotateObjectBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void RotateObjectBehavior::Start()
    {
        ViewportInputBehavior::Start();

        m_objectEntityId = m_controller->GetObjectEntityId();
        AZ_Assert(m_objectEntityId.IsValid(), "Failed to find m_objectEntityId");

        AZ::EntityId cameraEntityId = m_controller->GetCameraEntityId();
        AZ_Assert(cameraEntityId.IsValid(), "Failed to find cameraEntityId");

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        m_cameraRight = transform.GetBasisX();
        m_cameraUp = transform.GetBasisZ();
    }

    void RotateObjectBehavior::TickInternal(float x, float y, float z)
    {
        ViewportInputBehavior::TickInternal(x, y, z);

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_objectEntityId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformBus::Event(m_objectEntityId, &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternion(
                AZ::Quaternion::CreateFromAxisAngle(m_cameraUp, x) *
                AZ::Quaternion::CreateFromAxisAngle(m_cameraRight, y)) *
                transform);
    }

    float RotateObjectBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float RotateObjectBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

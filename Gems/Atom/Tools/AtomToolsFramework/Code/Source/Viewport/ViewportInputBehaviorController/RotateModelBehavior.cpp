/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateModelBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>

namespace AtomToolsFramework
{
    RotateModelBehavior::RotateModelBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void RotateModelBehavior::Start()
    {
        ViewportInputBehavior::Start();

        m_targetEntityId = m_controller->GetTargetEntityId();
        AZ_Assert(m_targetEntityId.IsValid(), "Failed to find m_targetEntityId");

        AZ::EntityId cameraEntityId = m_controller->GetCameraEntityId();
        AZ_Assert(cameraEntityId.IsValid(), "Failed to find cameraEntityId");

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        m_cameraRight = transform.GetBasisX();
    }

    void RotateModelBehavior::TickInternal(float x, float y, float z)
    {
        ViewportInputBehavior::TickInternal(x, y, z);

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_targetEntityId, &AZ::TransformBus::Events::GetLocalTM);

        AZ::Quaternion rotation = transform.GetRotation();
        rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), x) *
            AZ::Quaternion::CreateFromAxisAngle(m_cameraRight, y) * rotation;
        rotation.Normalize();

        AZ::TransformBus::Event(m_targetEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
    }

    float RotateModelBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float RotateModelBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

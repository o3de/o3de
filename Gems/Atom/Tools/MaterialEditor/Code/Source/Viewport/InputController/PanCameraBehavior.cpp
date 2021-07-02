/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Viewport/InputController/MaterialEditorViewportInputControllerBus.h>
#include <Source/Viewport/InputController/PanCameraBehavior.h>

namespace MaterialEditor
{
    void PanCameraBehavior::End()
    {
        float distanceToTarget;
        MaterialEditorViewportInputControllerRequestBus::BroadcastResult(
            distanceToTarget,
            &MaterialEditorViewportInputControllerRequestBus::Handler::GetDistanceToTarget);
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 targetPosition =
            transform.GetTranslation() +
            transform.GetBasisY() * distanceToTarget;
        MaterialEditorViewportInputControllerRequestBus::Broadcast(
            &MaterialEditorViewportInputControllerRequestBus::Handler::SetTargetPosition,
            targetPosition);
    }

    void PanCameraBehavior::TickInternal(float x, float y, [[maybe_unused]] float z)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Quaternion rotation = transform.GetRotation();
        const AZ::Vector3 right = transform.GetBasisX();
        rotation =
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), -x) *
            AZ::Quaternion::CreateFromAxisAngle(right, -y) *
            rotation;
        rotation.Normalize();
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
    }

    float PanCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float PanCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace MaterialEditor

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Viewport/InputController/MaterialEditorViewportInputControllerBus.h>
#include <Source/Viewport/InputController/MoveCameraBehavior.h>

namespace MaterialEditor
{
    void MoveCameraBehavior::End()
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

        Behavior::TickInternal(x, y, z);
    }

    float MoveCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float MoveCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace MaterialEditor

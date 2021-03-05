/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Viewport/InputController/MaterialEditorViewportInputControllerBus.h>
#include <Source/Viewport/InputController/MoveCameraBehavior.h>

namespace MaterialEditor
{
    void MoveCameraBehavior::Start()
    {
        Behavior::Start();

        MaterialEditorViewportInputControllerRequestBus::BroadcastResult(
            m_cameraEntityId,
            &MaterialEditorViewportInputControllerRequestBus::Handler::GetCameraEntityId);
        AZ_Assert(m_cameraEntityId.IsValid(), "Failed to find m_cameraEntityId");
    }

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

    void MoveCameraBehavior::TickInternal(float x, float y)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 up = transform.GetBasisZ();
        AZ::Vector3 right = transform.GetBasisX();
        AZ::Vector3 position = transform.GetTranslation();
        position +=
            up * y +
            right * -x;
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);
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

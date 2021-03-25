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

    void PanCameraBehavior::TickInternal(float x, float y, float z)
    {
        Behavior::TickInternal(x, y, z);

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

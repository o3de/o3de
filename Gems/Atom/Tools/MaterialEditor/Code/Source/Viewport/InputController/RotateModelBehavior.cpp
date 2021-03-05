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

#include <AzCore/Component/TransformBus.h>
#include <Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Viewport/InputController/RotateModelBehavior.h>

namespace MaterialEditor
{
    void RotateModelBehavior::Start()
    {
        Behavior::Start();

        MaterialEditorViewportInputControllerRequestBus::BroadcastResult(
            m_targetEntityId,
            &MaterialEditorViewportInputControllerRequestBus::Handler::GetTargetEntityId);
        AZ_Assert(m_targetEntityId.IsValid(), "Failed to find m_targetEntityId");
        AZ::EntityId cameraEntityId;
        MaterialEditorViewportInputControllerRequestBus::BroadcastResult(
            cameraEntityId,
            &MaterialEditorViewportInputControllerRequestBus::Handler::GetCameraEntityId);
        AZ_Assert(cameraEntityId.IsValid(), "Failed to find cameraEntityId");
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        m_cameraRight = transform.GetBasisX();
    }

    void RotateModelBehavior::TickInternal(float x, float y)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_targetEntityId, &AZ::TransformBus::Events::GetLocalTM);

        AZ::Quaternion rotation = transform.GetRotation();
        rotation =
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), x) *
            AZ::Quaternion::CreateFromAxisAngle(m_cameraRight, y) *
            rotation;
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
} // namespace MaterialEditor

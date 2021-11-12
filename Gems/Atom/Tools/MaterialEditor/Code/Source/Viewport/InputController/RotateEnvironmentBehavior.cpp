/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

#include <Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Viewport/InputController/RotateEnvironmentBehavior.h>

namespace MaterialEditor
{
    void RotateEnvironmentBehavior::Start()
    {
        Behavior::Start();

        MaterialEditorViewportInputControllerRequestBus::BroadcastResult(
            m_iblEntityId,
            &MaterialEditorViewportInputControllerRequestBus::Handler::GetIblEntityId);
        AZ_Assert(m_iblEntityId.IsValid(), "Failed to find m_iblEntityId");
        m_skyBoxFeatureProcessorInterface = AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::SkyBoxFeatureProcessorInterface>(m_iblEntityId);
    }

    void RotateEnvironmentBehavior::TickInternal(float x, float y, float z)
    {
        Behavior::TickInternal(x, y, z);

        m_rotation += x;
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), m_rotation);
        AZ::TransformBus::Event(m_iblEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateFromQuaternion(rotation);
        m_skyBoxFeatureProcessorInterface->SetCubemapRotationMatrix(rotationMatrix);
    }

    float RotateEnvironmentBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float RotateEnvironmentBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace MaterialEditor

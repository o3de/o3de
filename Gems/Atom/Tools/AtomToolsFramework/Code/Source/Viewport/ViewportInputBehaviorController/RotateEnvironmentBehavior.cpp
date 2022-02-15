/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>

namespace AtomToolsFramework
{
    RotateEnvironmentBehavior::RotateEnvironmentBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void RotateEnvironmentBehavior::Start()
    {
        ViewportInputBehavior::Start();

        m_environmentEntityId = m_controller->GetEnvironmentEntityId();
        AZ_Assert(m_environmentEntityId.IsValid(), "Failed to find m_environmentEntityId");
        m_skyBoxFeatureProcessor =
            AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::SkyBoxFeatureProcessorInterface>(m_environmentEntityId);
    }

    void RotateEnvironmentBehavior::TickInternal(float x, float y, float z)
    {
        ViewportInputBehavior::TickInternal(x, y, z);

        m_rotation += x;
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), m_rotation);
        AZ::TransformBus::Event(m_environmentEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateFromQuaternion(rotation);
        m_skyBoxFeatureProcessor->SetCubemapRotationMatrix(rotationMatrix);
    }

    float RotateEnvironmentBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float RotateEnvironmentBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

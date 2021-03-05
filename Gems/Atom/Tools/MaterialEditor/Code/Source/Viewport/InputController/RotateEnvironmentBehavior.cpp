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
        AZ::RPI::ScenePtr scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
        m_skyBoxFeatureProcessorInterface = scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
    }

    void RotateEnvironmentBehavior::TickInternal(float x, [[maybe_unused]] float y)
    {
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

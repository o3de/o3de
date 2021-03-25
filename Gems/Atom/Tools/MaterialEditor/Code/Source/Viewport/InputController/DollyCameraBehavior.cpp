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

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TransformBus.h>
#include <Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Viewport/InputController/DollyCameraBehavior.h>

namespace MaterialEditor
{
    void DollyCameraBehavior::TickInternal([[maybe_unused]] float x, float y, [[maybe_unused]] float z)
    {
        m_distanceToTarget = m_distanceToTarget + y;
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 position = m_targetPosition -
            transform.GetRotation().TransformVector(AZ::Vector3::CreateAxisY(m_distanceToTarget));
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);
    }

    float DollyCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float DollyCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace MaterialEditor

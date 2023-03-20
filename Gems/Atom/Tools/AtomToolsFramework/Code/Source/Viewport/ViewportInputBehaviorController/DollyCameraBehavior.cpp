/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector3.h>

namespace AtomToolsFramework
{
    DollyCameraBehavior::DollyCameraBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void DollyCameraBehavior::TickInternal([[maybe_unused]] float x, float y, [[maybe_unused]] float z)
    {
        m_objectDistance += y;
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 position = m_objectPosition - transform.GetRotation().TransformVector(AZ::Vector3::CreateAxisY(m_objectDistance));
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);
    }

    float DollyCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float DollyCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }
} // namespace AtomToolsFramework

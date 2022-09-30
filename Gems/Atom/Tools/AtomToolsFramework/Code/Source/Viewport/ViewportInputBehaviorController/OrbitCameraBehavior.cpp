/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>

namespace AtomToolsFramework
{
    OrbitCameraBehavior::OrbitCameraBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }

    void OrbitCameraBehavior::TickInternal(float x, float y, float z)
    {
        ViewportInputBehavior::TickInternal(x, y, z);

        // don't align camera until a movement has been made so that accidental right-click doesn't reset camera
        if (!m_aligned)
        {
            Align();
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::Quaternion rotation = transform.GetRotation();
        AZ::Vector3 right = transform.GetBasisX();
        rotation =
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), -x) * AZ::Quaternion::CreateFromAxisAngle(right, -y) * rotation;
        rotation.Normalize();
        AZ::Vector3 position = rotation.TransformVector(AZ::Vector3(0.0f, -m_objectDistance, 0.0f)) + m_objectPosition;
        transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, position);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTM, transform);
    }

    float OrbitCameraBehavior::GetSensitivityX()
    {
        return SensitivityX;
    }

    float OrbitCameraBehavior::GetSensitivityY()
    {
        return SensitivityY;
    }

    void OrbitCameraBehavior::Align()
    {
        AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
        const AZ::Vector3 delta = m_objectPosition - cameraPosition;
        AZ::Quaternion objectRotation = LookRotation(delta);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldRotationQuaternion, objectRotation);
        m_aligned = true;
    }
} // namespace AtomToolsFramework

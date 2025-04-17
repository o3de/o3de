/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector3.h>

namespace AtomToolsFramework
{
    ViewportInputBehavior::ViewportInputBehavior(ViewportInputBehaviorControllerInterface* controller)
        : m_controller(controller)
    {
        AZ::TickBus::Handler::BusConnect();
    }

    ViewportInputBehavior::~ViewportInputBehavior()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ViewportInputBehavior::Start()
    {
        m_x = {};
        m_y = {};
        m_z = {};

        m_cameraEntityId = m_controller->GetCameraEntityId();
        AZ_Assert(m_cameraEntityId.IsValid(), "Failed to find m_cameraEntityId");
        m_objectDistance = m_controller->GetObjectDistance();
        m_objectPosition = m_controller->GetObjectPosition();
        m_objectRadius = m_controller->GetObjectRadius();
    }

    void ViewportInputBehavior::End()
    {
    }

    void ViewportInputBehavior::MoveX(float value)
    {
        m_x += value * GetSensitivityX();
    }

    void ViewportInputBehavior::MoveY(float value)
    {
        m_y += value * GetSensitivityY();
    }

    void ViewportInputBehavior::MoveZ(float value)
    {
        m_z += value * GetSensitivityZ();
    }

    bool ViewportInputBehavior::HasDelta() const
    {
        return AZ::GetAbs(m_x) > std::numeric_limits<float>::min() || AZ::GetAbs(m_y) > std::numeric_limits<float>::min() ||
            AZ::GetAbs(m_z) > std::numeric_limits<float>::min();
    }

    void ViewportInputBehavior::TickInternal([[maybe_unused]] float x, [[maybe_unused]] float y, float z)
    {
        m_objectDistance -= z;

        bool isCameraCentered = m_controller->IsCameraCentered();

        // if camera is looking at the object (locked to the object) we don't want to zoom past the object's center
        if (isCameraCentered)
        {
            m_objectDistance = AZ::GetMax(m_objectDistance, 0.0f);
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTM);
        AZ::Vector3 position = m_objectPosition - transform.GetRotation().TransformVector(AZ::Vector3::CreateAxisY(m_objectDistance));
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);

        // if camera is not locked to the object, move its focal point so we can free look
        if (!isCameraCentered)
        {
            m_objectPosition += transform.GetRotation().TransformVector(AZ::Vector3::CreateAxisY(z));
            m_controller->SetObjectPosition(m_objectPosition);
            m_objectDistance = m_controller->GetObjectDistance();
        }
    }

    float ViewportInputBehavior::GetSensitivityX()
    {
        return 0;
    }

    float ViewportInputBehavior::GetSensitivityY()
    {
        return 0;
    }

    float ViewportInputBehavior::GetSensitivityZ()
    {
        // adjust zooming sensitivity by object size, so that large objects zoom at the same speed as smaller ones
        return 0.001f * AZ::GetMax(0.5f, m_objectRadius);
    }

    AZ::Quaternion ViewportInputBehavior::LookRotation(AZ::Vector3 forward)
    {
        forward.Normalize();
        AZ::Vector3 right = forward.CrossZAxis();
        right.Normalize();
        AZ::Vector3 up = right.Cross(forward);
        up.Normalize();
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromBasis(right, forward, up);
        rotation.Normalize();
        return rotation;
    }

    float ViewportInputBehavior::TakeStep(float& value, float t)
    {
        const float absValue = AZ::GetAbs(value);
        const float step = absValue < SnapInterval ? value : AZ::Lerp(0, value, t);
        value -= step;
        return step;
    }

    void ViewportInputBehavior::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // delta x and y values are accumulated in MoveX and MoveY functions (by dragging the mouse)
        // in the Tick function we then lerp them down to 0 over short time and apply delta transform to an entity
        if (HasDelta())
        {
            // t is a lerp amount based on time between frames (deltaTime)
            // GetMin restricts how much we can lerp in case of low fps (and very high deltaTime)
            const float t = AZ::GetMin(deltaTime / LerpTime, 0.5f);
            const float x = TakeStep(m_x, t);
            const float y = TakeStep(m_y, t);
            const float z = TakeStep(m_z, t);
            TickInternal(x, y, z);
        }
    }
} // namespace AtomToolsFramework

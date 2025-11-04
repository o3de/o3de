/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QApplication>
#include <QWidget>

namespace AtomToolsFramework
{
    ViewportInputBehaviorController::ViewportInputBehaviorController(
        QWidget* owner, const AZ::EntityId& cameraEntityId, const AZ::EntityId& objectEntityId, const AZ::EntityId& environmentEntityId)
        : AzFramework::SingleViewportController()
        , m_owner(owner)
        , m_cameraEntityId(cameraEntityId)
        , m_objectEntityId(objectEntityId)
        , m_environmentEntityId(environmentEntityId)
        , m_objectPosition(AZ::Vector3::CreateZero())
    {
    }

    ViewportInputBehaviorController::~ViewportInputBehaviorController()
    {
    }

    void ViewportInputBehaviorController::AddBehavior(KeyMask mask, AZStd::shared_ptr<ViewportInputBehavior> behavior)
    {
        m_behaviorMap[mask] = behavior;
    }

    const AZ::EntityId& ViewportInputBehaviorController::GetCameraEntityId() const
    {
        return m_cameraEntityId;
    }

    const AZ::EntityId& ViewportInputBehaviorController::GetObjectEntityId() const
    {
        return m_objectEntityId;
    }

    const AZ::EntityId& ViewportInputBehaviorController::GetEnvironmentEntityId() const
    {
        return m_environmentEntityId;
    }

    const AZ::Vector3& ViewportInputBehaviorController::GetObjectPosition() const
    {
        return m_objectPosition;
    }

    void ViewportInputBehaviorController::SetObjectPosition(const AZ::Vector3& objectPosition)
    {
        m_objectPosition = objectPosition;
        m_isCameraCentered = false;
    }

    float ViewportInputBehaviorController::GetObjectRadiusMin() const
    {
        return 0.5f;
    }

    float ViewportInputBehaviorController::GetObjectRadius() const
    {
        float radius = 0.0f;
        AZ::Vector3 center = AZ::Vector3::CreateZero();
        m_objectBounds.GetAsSphere(center, radius);
        return AZ::GetMax(radius, GetObjectRadiusMin());
    }

    float ViewportInputBehaviorController::GetObjectDistance() const
    {
        AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
        return cameraPosition.GetDistance(m_objectPosition);
    }

    void ViewportInputBehaviorController::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (m_keysChanged)
        {
            if (m_timeToBehaviorSwitchMs > 0)
            {
                m_timeToBehaviorSwitchMs -= event.m_deltaTime.count();
            }
            if (m_timeToBehaviorSwitchMs <= 0)
            {
                EvaluateControlBehavior();
                m_keysChanged = false;
            }
        }
    }

    bool ViewportInputBehaviorController::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        using namespace AzFramework;

        const InputChannelId& inputChannelId = event.m_inputChannel.GetInputChannelId();
        const InputChannel::State state = event.m_inputChannel.GetState();
        const KeyMask keysOld = m_keys;

        bool mouseOver = false;
        AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::EventResult(
            mouseOver, GetViewportId(), &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::IsMouseOver);

        if (!m_behavior)
        {
            EvaluateControlBehavior();
        }

        switch (state)
        {
        case InputChannel::State::Began:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys |= Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys |= Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys |= Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys |= Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys |= Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys |= Shift;
            }
            if (m_behavior && inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(event.m_inputChannel.GetValue());
            }
            else if (m_behavior && inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(event.m_inputChannel.GetValue());
            }
            else if (m_behavior && inputChannelId == InputDeviceMouse::Movement::Z && mouseOver)
            {
                m_behavior->MoveZ(event.m_inputChannel.GetValue());
            }
            break;
        case InputChannel::State::Ended:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys &= ~Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys &= ~Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys &= ~Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys &= ~Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys &= ~Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys &= ~Shift;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::AlphanumericZ && (m_keys & Ctrl) == None)
            {
                // only reset camera if no other widget is in focus
                if (!QApplication::focusWidget() || m_owner->hasFocus())
                {
                    Reset();
                }
            }
            break;
        case InputChannel::State::Updated:
            if (m_behavior && inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(event.m_inputChannel.GetValue());
            }
            else if (m_behavior && inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(event.m_inputChannel.GetValue());
            }
            else if (m_behavior && inputChannelId == InputDeviceMouse::Movement::Z && mouseOver)
            {
                m_behavior->MoveZ(event.m_inputChannel.GetValue());
            }
            break;
        }

        if (keysOld != m_keys)
        {
            m_keysChanged = true;
            m_timeToBehaviorSwitchMs = BehaviorSwitchDelayMs;
        }
        return false;
    }

    void ViewportInputBehaviorController::Reset()
    {
        // Reset environment entity transform
        AZ::TransformBus::Event(m_environmentEntityId, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateIdentity());

        // Reset object entity transform
        AZ::TransformBus::Event(m_objectEntityId, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateIdentity());

        // Get the world bounds from the object entity. If the bounds are invalid then use bounds with the default position and radius.
        m_objectBounds = AZ::Aabb::CreateNull();
        AzFramework::BoundsRequestBus::EventResult(
            m_objectBounds, m_objectEntityId, &AzFramework::BoundsRequestBus::Events::GetWorldBounds);
        if (!m_objectBounds.IsValid() || !m_objectBounds.IsFinite())
        {
            m_objectBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), GetObjectRadiusMin());
        }

        // Reset the camera to target the object entity with the default offset and rotation.
        m_objectPosition = m_objectBounds.GetCenter();
        const float distanceMin = GetObjectRadius() * 0.5f + DepthNear;
        const float distance = distanceMin * StartingDistanceMultiplier;
        const AZ::Quaternion cameraRotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), StartingRotationAngle);
        const AZ::Vector3 cameraPosition = m_objectPosition - cameraRotation.TransformVector(AZ::Vector3::CreateAxisY() * distance);
        const AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTM, cameraTransform);
        m_isCameraCentered = true;

        if (m_behavior)
        {
            m_behavior->End();
            m_behavior->Start();
        }
    }

    void ViewportInputBehaviorController::SetFieldOfView(float value)
    {
        Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraRequestBus::Events::SetFovDegrees, value);
    }

    bool ViewportInputBehaviorController::IsCameraCentered() const
    {
        return m_isCameraCentered;
    }

    void ViewportInputBehaviorController::EvaluateControlBehavior()
    {
        auto it = m_behaviorMap.find(m_keys);
        if (it == m_behaviorMap.end())
        {
            it = m_behaviorMap.find(None);
        }

        AZStd::shared_ptr<ViewportInputBehavior> nextBehavior =
            it != m_behaviorMap.end() ? it->second : AZStd::shared_ptr<ViewportInputBehavior>();

        if (m_behavior != nextBehavior)
        {
            if (m_behavior)
            {
                m_behavior->End();
            }

            m_behavior = nextBehavior;

            if (m_behavior)
            {
                m_behavior->Start();
            }
        }
    }
} // namespace AtomToolsFramework

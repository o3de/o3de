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
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QApplication>
#include <QWidget>

namespace AtomToolsFramework
{
    ViewportInputBehaviorController::ViewportInputBehaviorController(
        const AZ::EntityId& cameraEntityId, const AZ::EntityId& targetEntityId, const AZ::EntityId& environmentEntityId)
        : AzFramework::SingleViewportController()
        , m_cameraEntityId(cameraEntityId)
        , m_targetEntityId(targetEntityId)
        , m_environmentEntityId(environmentEntityId)
        , m_targetPosition(AZ::Vector3::CreateZero())
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

    const AZ::EntityId& ViewportInputBehaviorController::GetTargetEntityId() const
    {
        return m_targetEntityId;
    }

    const AZ::EntityId& ViewportInputBehaviorController::GetEnvironmentEntityId() const
    {
        return m_environmentEntityId;
    }

    const AZ::Vector3& ViewportInputBehaviorController::GetTargetPosition() const
    {
        return m_targetPosition;
    }

    void ViewportInputBehaviorController::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        m_targetPosition = targetPosition;
        m_isCameraCentered = false;
    }

    void ViewportInputBehaviorController::SetTargetBounds(const AZ::Aabb& targetBounds)
    {
        m_targetBounds = targetBounds;
    }

    float ViewportInputBehaviorController::GetDistanceToTarget() const
    {
        AZ::Vector3 cameraPosition;
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
        return cameraPosition.GetDistance(m_targetPosition);
    }

    void ViewportInputBehaviorController::GetExtents(float& distanceMin, float& distanceMax) const
    {
        distanceMin = m_distanceMin;
        distanceMax = m_distanceMax;
    }

    float ViewportInputBehaviorController::GetRadius() const
    {
        return m_radius;
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
                // only reset camera if no other widget besides viewport is in focus
                const auto focus = QApplication::focusWidget();
                if (!focus || focus->objectName() == "Viewport")
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
        CalculateExtents();

        // reset camera
        m_targetPosition = m_modelCenter;
        const float distance = m_distanceMin * StartingDistanceMultiplier;
        const AZ::Quaternion cameraRotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), StartingRotationAngle);
        AZ::Vector3 cameraPosition(m_targetPosition.GetX(), m_targetPosition.GetY() - distance, m_targetPosition.GetZ());
        cameraPosition = cameraRotation.TransformVector(cameraPosition);
        AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTM, cameraTransform);
        m_isCameraCentered = true;

        // reset model
        AZ::Transform modelTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_targetEntityId, &AZ::TransformBus::Events::SetLocalTM, modelTransform);

        // reset environment
        AZ::Transform environmentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_environmentEntityId, &AZ::TransformBus::Events::SetLocalTM, environmentTransform);

        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateIdentity();
        auto skyBoxFeatureProcessor =
            AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::SkyBoxFeatureProcessorInterface>(m_environmentEntityId);
        skyBoxFeatureProcessor->SetCubemapRotationMatrix(rotationMatrix);

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

    void ViewportInputBehaviorController::CalculateExtents()
    {
        AZ::TransformBus::EventResult(m_modelCenter, m_targetEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
        m_targetBounds.GetAsSphere(m_modelCenter, m_radius);
        m_distanceMin = m_targetBounds.GetExtents().GetMinElement() * 0.5f + DepthNear;
        m_distanceMax = m_radius * MaxDistanceMultiplier;
    }

    void ViewportInputBehaviorController::EvaluateControlBehavior()
    {
        auto it = m_behaviorMap.find(m_keys);
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

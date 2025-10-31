/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QWidget>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>

#include <InputController/OpenParticleEditorViewportInputController.h>
#include <InputController/IdleBehavior.h>
#include <InputController/PanCameraBehavior.h>
#include <InputController/OrbitCameraBehavior.h>
#include <InputController/MoveCameraBehavior.h>
#include <InputController/DollyCameraBehavior.h>


namespace OpenParticleSystemEditor
{
    OpenParticleEditorViewportInputController::OpenParticleEditorViewportInputController()
        : AzFramework::SingleViewportController()
        , m_targetPosition(AZ::Vector3::CreateZero())
    {
        m_behaviorMap[None] = AZStd::make_shared<IdleBehavior>();
        m_behaviorMap[Lmb] = AZStd::make_shared<PanCameraBehavior>();
        m_behaviorMap[Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Rmb] = AZStd::make_shared<OrbitCameraBehavior>();
        m_behaviorMap[Alt ^ Lmb] = AZStd::make_shared<OrbitCameraBehavior>();
        m_behaviorMap[Alt ^ Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Alt ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
        m_behaviorMap[Lmb ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
    }

    OpenParticleEditorViewportInputController::~OpenParticleEditorViewportInputController()
    {
        if (m_initialized)
        {
            OpenParticleEditorViewportInputControllerRequestBus::Handler::BusDisconnect();
        }
    }

    void OpenParticleEditorViewportInputController::Init(const AZ::EntityId& cameraEntityId)
    {
        if (m_initialized)
        {
            AZ_Error("OpenParticleEditorViewportInputController", false, "Controller already initialized.");
            return;
        }
        m_initialized = true;
        m_cameraEntityId = cameraEntityId;

        OpenParticleEditorViewportInputControllerRequestBus::Handler::BusConnect();
    }

    const AZ::EntityId& OpenParticleEditorViewportInputController::GetCameraEntityId() const
    {
        return m_cameraEntityId;
    }

    const AZ::EntityId& OpenParticleEditorViewportInputController::GetTargetEntityId() const
    {
        return m_targetEntityId;
    }

    const AZ::EntityId& OpenParticleEditorViewportInputController::GetIblEntityId() const
    {
        return m_iblEntityId;
    }

    const AZ::Vector3& OpenParticleEditorViewportInputController::GetTargetPosition() const
    {
        return m_targetPosition;
    }

    void OpenParticleEditorViewportInputController::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        m_targetPosition = targetPosition;
        m_isCameraCentered = false;
    }

    float OpenParticleEditorViewportInputController::GetDistanceToTarget() const
    {
        AZ::Vector3 cameraPosition;
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
        return cameraPosition.GetDistance(m_targetPosition);
    }

    void OpenParticleEditorViewportInputController::GetExtents(float& distanceMin, float& distanceMax) const
    {
        distanceMin = m_distanceMin;
        distanceMax = m_distanceMax;
    }

    float OpenParticleEditorViewportInputController::GetRadius() const
    {
        return m_radius;
    }

    void OpenParticleEditorViewportInputController::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
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

    bool OpenParticleEditorViewportInputController::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        using namespace AzFramework;

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
            HandleStateBegan(event, mouseOver);
            break;
        case InputChannel::State::Ended:
            HandleStateEnded(event);
            break;
        case InputChannel::State::Updated:
            HandleStateUpdated(event, mouseOver);
            break;
        default:
            break;
        }

        if (keysOld != m_keys)
        {
            m_keysChanged = true;
            m_timeToBehaviorSwitchMs = BehaviorSwitchDelayMs;
        }
        return false;
    }

    void OpenParticleEditorViewportInputController::Reset()
    {
        // reset camera
        m_targetPosition = GetDefaultPosition();
        const AZ::Quaternion cameraRotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), StartingRotationAngle);
        AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(cameraRotation, GetStartingCameraPosition());
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTM, cameraTransform);
        m_isCameraCentered = true;

        if (m_behavior)
        {
            m_behavior->End();
            m_behavior->Start();
        }
    }

    void OpenParticleEditorViewportInputController::SetFieldOfView(float value)
    {
        Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraRequestBus::Events::SetFovDegrees, value);
    }

    bool OpenParticleEditorViewportInputController::IsCameraCentered() const
    {
        return m_isCameraCentered;
    }

    void OpenParticleEditorViewportInputController::EvaluateControlBehavior()
    {
        AZStd::shared_ptr<Behavior> nextBehavior;
        auto it = m_behaviorMap.find(m_keys);
        if (it == m_behaviorMap.end())
        {
            nextBehavior = m_behaviorMap[None];
        }
        else
        {
            nextBehavior = it->second;
        }

        if (nextBehavior == m_behavior)
        {
            return;
        }

        if (m_behavior)
        {
            m_behavior->End();
        }
        m_behavior = nextBehavior;
        m_behavior->Start();
    }

    void OpenParticleEditorViewportInputController::HandleStateBegan(
        const AzFramework::ViewportControllerInputEvent& event, const bool mouseOver)
    {
        using namespace AzFramework;
        const InputChannelId& inputChannelId = event.m_inputChannel.GetInputChannelId();

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
        if (inputChannelId == InputDeviceMouse::Movement::X)
        {
            m_behavior->MoveX(event.m_inputChannel.GetValue());
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Y)
        {
            m_behavior->MoveY(event.m_inputChannel.GetValue());
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            if (mouseOver)
            {
                m_behavior->MoveZ(event.m_inputChannel.GetValue());
            }
        }
    }

    void OpenParticleEditorViewportInputController::HandleStateEnded(const AzFramework::ViewportControllerInputEvent& event)
    {
        using namespace AzFramework;
        const InputChannelId& inputChannelId = event.m_inputChannel.GetInputChannelId();
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
            if (!focus || focus->objectName() == "OpenParticleViewportWidget")
            {
                Reset();
            }
        }
    }

    void OpenParticleEditorViewportInputController::HandleStateUpdated(
        const AzFramework::ViewportControllerInputEvent& event, const bool mouseOver)
    {
        using namespace AzFramework;

        const InputChannelId& inputChannelId = event.m_inputChannel.GetInputChannelId();
        if (inputChannelId == InputDeviceMouse::Movement::X)
        {
            m_behavior->MoveX(event.m_inputChannel.GetValue());
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Y)
        {
            m_behavior->MoveY(event.m_inputChannel.GetValue());
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            if (mouseOver)
            {
                m_behavior->MoveZ(event.m_inputChannel.GetValue());
            }
        }
    }
} // namespace OpenParticleSystemEditor

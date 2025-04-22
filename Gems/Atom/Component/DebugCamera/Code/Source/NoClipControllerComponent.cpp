/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>

#include <AzCore/Component/TransformBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Components/CameraBus.h>

#include <DebugCameraUtils.h>

namespace AZ
{
    namespace Debug
    {
        const AzFramework::InputChannelId NoClipControllerComponent::TouchEvent::InvalidTouchChannelId = AzFramework::InputChannelId("InvalidChannel");

        static constexpr float MaxFov = 160.0f * Constants::Pi / 180.0f;
        static constexpr float MinFov = 1.0f * Constants::Pi / 180.0f;
        static constexpr float DefaultFov = Constants::QuarterPi;
        static constexpr float deadZone = 0.07f;

        void NoClipControllerProperties::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<NoClipControllerProperties>()
                    ->Version(2)
                    ->Field("Mouse Sensitivity X", &NoClipControllerProperties::m_mouseSensitivityX)
                    ->Field("Mouse Sensitivity Y", &NoClipControllerProperties::m_mouseSensitivityY)
                    ->Field("Move Speed", &NoClipControllerProperties::m_moveSpeed)
                    ->Field("Panning Speed", &NoClipControllerProperties::m_panningSpeed)
                    ->Field("Touch Sensitivity", &NoClipControllerProperties::m_touchSensitivity);
            }
        }

        NoClipControllerComponent::NoClipControllerComponent()
            : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDefault())
        {}

        void NoClipControllerComponent::Reflect(AZ::ReflectContext* reflection)
        {
            NoClipControllerProperties::Reflect(reflection);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<NoClipControllerComponent, CameraControllerComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Properties", &NoClipControllerComponent::m_properties);
            }
        }

        void NoClipControllerComponent::OnEnabled()
        {
            // Reset parameters
            m_mouseLookEnabled = false;
            m_inputStates = 0;
            m_currentHeading = 0.0f;
            m_currentPitch = 0.0f;
            m_currentFov = DefaultFov;
            m_lastForward = 0.0f;
            m_lastStrafe = 0.0f;
            m_lastAscent = 0.0f;

            NoClipControllerRequestBus::Handler::BusConnect(GetEntityId());
            AzFramework::InputChannelEventListener::Connect();
            AZ::TickBus::Handler::BusConnect();
        }

        void NoClipControllerComponent::OnDisabled()
        {
            TickBus::Handler::BusDisconnect();
            AzFramework::InputChannelEventListener::Disconnect();
            NoClipControllerRequestBus::Handler::BusDisconnect();

            // Reset the Fov to default.
            Camera::CameraRequestBus::Event(GetEntityId(), &Camera::CameraRequestBus::Events::SetFovRadians, DefaultFov);
        }

        void NoClipControllerComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            static const float normalSpeed = 3.0f;
            static const float sprintSpeed = 10.0f;

            float speedFactor = m_inputStates[CameraKeys::FastMode] ? sprintSpeed : normalSpeed;

            float forward = m_properties.m_moveSpeed * speedFactor * (
                (m_inputStates[CameraKeys::Forward] ? deltaTime : 0.0f) +
                (m_inputStates[CameraKeys::Back] ? -deltaTime : 0.0f));

            float strafe = m_properties.m_panningSpeed * speedFactor * (
                (m_inputStates[CameraKeys::Right] ? deltaTime : 0.0f) +
                (m_inputStates[CameraKeys::Left] ? -deltaTime : 0.0f));

            float ascent = m_properties.m_panningSpeed * speedFactor * (
                (m_inputStates[CameraKeys::Up] ? deltaTime : 0.0f) +
                (m_inputStates[CameraKeys::Down] ? -deltaTime : 0.0f));

            ApplyMomentum(m_lastForward, forward, deltaTime);
            ApplyMomentum(m_lastStrafe, strafe, deltaTime);
            ApplyMomentum(m_lastAscent, ascent, deltaTime);

            AZ::Vector3 worldPosition;
            AZ::TransformBus::EventResult(
                worldPosition, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

            // The coordinate system is right-handed and Z-up. So heading is a rotation around the Z axis.
            // After that rotation we rotate around the (rotated by heading) X axis for pitch.
            AZ::Quaternion orientation = AZ::Quaternion::CreateRotationZ(m_currentHeading)
                * AZ::Quaternion::CreateRotationX(m_currentPitch);
            AZ::Vector3 position = orientation.TransformVector(AZ::Vector3(strafe, forward, ascent)) + worldPosition;

            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(orientation, position);
            AZ::TransformBus::Event(
                GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, transform);

            Camera::CameraRequestBus::Event(GetEntityId(), &Camera::CameraRequestBus::Events::SetFovRadians, m_currentFov);
        }

        bool NoClipControllerComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
        {
            static const auto KeyCount = static_cast<uint32_t>(CameraKeys::Count);
            static const float PixelToDegree = 1.0 / 360.0f;

            static const AzFramework::InputChannelId CameraInputMap[KeyCount] =
            {
                AzFramework::InputDeviceKeyboard::Key::AlphanumericW,  // Forward
                AzFramework::InputDeviceKeyboard::Key::AlphanumericS,  // Back
                AzFramework::InputDeviceKeyboard::Key::AlphanumericA,  // Left
                AzFramework::InputDeviceKeyboard::Key::AlphanumericD,  // Right
                AzFramework::InputDeviceKeyboard::Key::AlphanumericQ,  // Up
                AzFramework::InputDeviceKeyboard::Key::AlphanumericE,  // Down
                AzFramework::InputDeviceKeyboard::Key::ModifierShiftL, // FastMode
            };

            static const AzFramework::InputChannelId CameraGamepadInputMap[KeyCount] =
            {
                AzFramework::InputDeviceGamepad::Button::DU,  // Forward
                AzFramework::InputDeviceGamepad::Button::DD,  // Back
                AzFramework::InputDeviceGamepad::Button::DL,  // Left
                AzFramework::InputDeviceGamepad::Button::DR,  // Right
                AzFramework::InputDeviceGamepad::Button::R1,  // Up
                AzFramework::InputDeviceGamepad::Button::L1,  // Down
                AzFramework::InputDeviceGamepad::Trigger::R2, // FastMode
            };

            uint32_t handledChannels = NoClipControllerChannel_None;

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            switch (inputChannel.GetState())
            {
            case AzFramework::InputChannel::State::Began:
            case AzFramework::InputChannel::State::Updated: // update the camera rotation
            {
                //Keyboard & Mouse
                if (m_mouseLookEnabled && inputChannelId == AzFramework::InputDeviceMouse::Movement::X)
                {
                    // modify yaw angle
                    m_currentHeading -= inputChannel.GetValue() * m_properties.m_mouseSensitivityX * PixelToDegree;
                    m_currentHeading = NormalizeAngle(m_currentHeading);
                }
                else if (m_mouseLookEnabled && inputChannelId == AzFramework::InputDeviceMouse::Movement::Y)
                {
                    // modify pitch angle
                    m_currentPitch -= inputChannel.GetValue() * m_properties.m_mouseSensitivityY * PixelToDegree;
                    m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                    m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    // modify field of view
                    m_currentFov = GetClamp(m_currentFov - inputChannel.GetValue() * 0.0005f * m_currentFov, MinFov, MaxFov);
                    handledChannels |= NoClipControllerChannel_Fov;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    m_mouseLookEnabled = true;
                    handledChannels |= NoClipControllerChannel_Orientation;
                }
                else
                {
                    for (uint32_t i = 0; i < KeyCount; ++i)
                    {
                        if (inputChannelId == CameraInputMap[i])
                        {
                            m_inputStates[i] = true;
                            if (i != CameraKeys::FastMode)
                            {
                                handledChannels |= NoClipControllerChannel_Position;
                            }
                            break;
                        }
                    }
                }
                // Gamepad
                if (inputChannelId == AzFramework::InputDeviceGamepad::Trigger::L2)
                {
                    m_mouseLookEnabled = true;
                    handledChannels |= NoClipControllerChannel_Orientation;
                }
                else if (m_mouseLookEnabled)
                {
                    if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RX)
                    {
                        // modify yaw angle
                        m_currentHeading -= inputChannel.GetValue() * m_properties.m_mouseSensitivityX * PixelToDegree;
                        m_currentHeading = NormalizeAngle(m_currentHeading);
                    }
                    else if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RY)
                    {
                        // modify pitch angle
                        m_currentPitch += inputChannel.GetValue() * m_properties.m_mouseSensitivityY * PixelToDegree;
                        m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                        m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
                    }

                    for (uint32_t i = 0; i < KeyCount; ++i)
                    {
                        if (inputChannelId == CameraGamepadInputMap[i])
                        {
                            m_inputStates[i] = true;
                            if (i != CameraKeys::FastMode)
                            {
                                handledChannels |= NoClipControllerChannel_Position;
                            }
                            break;
                        }
                    }
                }

                // Touch controls works like two virtual joysticks.
                // The left "joystick" controls forward/backward/left and right movements.
                // The right "joystick" controls the camera heading and pitch.
                // There's no control to move up and down.
                if (inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0 || inputChannelId == AzFramework::InputDeviceTouch::Touch::Index1)
                {
                    auto* positionData = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ::Vector2 screenPos = positionData->m_normalizedPosition;
                    if (inputChannelId == m_mouseLookTouch.m_channelId)
                    {
                        // modify yaw angle
                        AZ::Vector2 deltaPos = screenPos - m_mouseLookTouch.m_initialPos;
                        AZ::Vector2 inputValue = AZ::Vector2(fabsf(deltaPos.GetX()) > deadZone ? 1.0f : 0, fabsf(deltaPos.GetY()) > deadZone ? 1.0f : 0);
                        m_currentHeading -= inputValue.GetX() * AZ::GetSign(deltaPos.GetX()) * m_properties.m_touchSensitivity * m_properties.m_mouseSensitivityX * PixelToDegree;
                        m_currentHeading = NormalizeAngle(m_currentHeading);

                        // modify pitch angle
                        m_currentPitch -= inputValue.GetY() * AZ::GetSign(deltaPos.GetY()) * m_properties.m_touchSensitivity * m_properties.m_mouseSensitivityY * PixelToDegree;
                        m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                        m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);

                        handledChannels |= NoClipControllerChannel_Orientation;
                    }
                    else if (inputChannelId == m_movementTouch.m_channelId)
                    {
                        AZ::Vector2 deltaPos = screenPos - m_movementTouch.m_initialPos;
                        m_inputStates[Forward] = deltaPos.GetY() < -deadZone ? true : false;
                        m_inputStates[Back] = deltaPos.GetY() > deadZone ? true : false;
                        m_inputStates[Left] = deltaPos.GetX() < -deadZone ? true : false;
                        m_inputStates[Right] = deltaPos.GetX() > deadZone ? true : false;

                        handledChannels |= NoClipControllerChannel_Position;
                    }
                    else
                    {
                        bool isMouseLook = (screenPos.GetX() > 0.5);

                        auto& touchEvent = isMouseLook ? m_mouseLookTouch : m_movementTouch;
                        if (touchEvent.m_channelId == TouchEvent::InvalidTouchChannelId)
                        {
                            touchEvent.m_channelId = inputChannelId;
                            touchEvent.m_initialPos = screenPos;
                        }

                        if (isMouseLook)
                        {
                            handledChannels |= NoClipControllerChannel_Orientation;
                        }
                        else
                        {
                            handledChannels |= NoClipControllerChannel_Position;
                        }
                    }
                }

                if (handledChannels && AzFramework::InputChannel::State::Began == inputChannel.GetState())
                {
                    CameraControllerNotificationBus::Broadcast(&CameraControllerNotifications::OnCameraMoveBegan, RTTI_GetType(), handledChannels);
                }

                break;
            }
            case AzFramework::InputChannel::State::Ended: // update the released input state
            {
                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    m_mouseLookEnabled = false;
                    handledChannels |= NoClipControllerChannel_Orientation;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    handledChannels |= NoClipControllerChannel_Fov;
                }
                else if (inputChannelId == AzFramework::InputDeviceGamepad::Trigger::L2)
                {
                    m_mouseLookEnabled = false;
                    handledChannels |= NoClipControllerChannel_Orientation;
                    // On gamepads, Trigger::L2 also indicates positional movement, see above.
                    handledChannels |= NoClipControllerChannel_Position;
                }
                else if (inputChannelId == m_movementTouch.m_channelId)
                {
                    m_movementTouch.m_channelId = TouchEvent::InvalidTouchChannelId;
                    for (uint32_t i = 0; i < KeyCount; ++i)
                    {
                        m_inputStates[i] = false;
                        if (i != CameraKeys::FastMode)
                        {
                            handledChannels |= NoClipControllerChannel_Position;
                        }
                    }
                }
                else if (inputChannelId == m_mouseLookTouch.m_channelId)
                {
                    m_mouseLookTouch.m_channelId = TouchEvent::InvalidTouchChannelId;
                    handledChannels |= NoClipControllerChannel_Orientation;
                }
                else
                {
                    for (uint32_t i = 0; i < KeyCount; ++i)
                    {
                        if (inputChannelId == CameraInputMap[i] || inputChannelId == CameraGamepadInputMap[i])
                        {
                            m_inputStates[i] = false;
                            if (i != CameraKeys::FastMode)
                            {
                                handledChannels |= NoClipControllerChannel_Position;
                            }
                            break;
                        }
                    }
                }

                if (handledChannels)
                {
                    CameraControllerNotificationBus::Broadcast(&CameraControllerNotifications::OnCameraMoveEnded, RTTI_GetType(), handledChannels);
                }

                break;
            }
            default:
            {
                break;
            }
            }
            return false;
        }

        void NoClipControllerComponent::SetMouseSensitivityX(float mouseSensitivityX)
        {
            m_properties.m_mouseSensitivityX = mouseSensitivityX;
        }
        void NoClipControllerComponent::SetMouseSensitivityY(float mouseSensitivityY)
        {
            m_properties.m_mouseSensitivityY = mouseSensitivityY;
        }
        void NoClipControllerComponent::SetMoveSpeed(float moveSpeed)
        {
            m_properties.m_moveSpeed = moveSpeed;
        }
        void NoClipControllerComponent::SetPanningSpeed(float panningSpeed)
        {
            m_properties.m_panningSpeed = panningSpeed;
        }
        void NoClipControllerComponent::SetControllerProperties(const NoClipControllerProperties& properties)
        {
            m_properties = properties;
        }

        void NoClipControllerComponent::SetTouchSensitivity(float touchSensitivity)
        {
            m_properties.m_touchSensitivity = touchSensitivity;
        }

        void NoClipControllerComponent::SetPosition(AZ::Vector3 position)
        {
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, position);
        }

        void NoClipControllerComponent::SetHeading(float heading)
        {
            m_currentHeading = heading;
            m_currentHeading = NormalizeAngle(m_currentHeading);
        }

        void NoClipControllerComponent::SetCameraStateForward(float value)
        {
            m_inputStates[CameraKeys::Forward] = value >= -deadZone; 
        }

        void NoClipControllerComponent::SetCameraStateBack(float value)
        {
            m_inputStates[Back] = value <= deadZone;
        }

        void NoClipControllerComponent::SetCameraStateLeft(float value)
        {
            m_inputStates[Left] = value < -deadZone; 
        }

        void NoClipControllerComponent::SetCameraStateRight(float value)
        {
            m_inputStates[Right] = value > deadZone;
        }

        void NoClipControllerComponent::SetCameraStateUp(float value)
        {
            m_inputStates[Up] = value > deadZone;
        }

        void NoClipControllerComponent::SetCameraStateDown(float value)
        {
            m_inputStates[Down] = value > deadZone;
        }

        void NoClipControllerComponent::SetPitch(float pitch)
        {
            m_currentPitch = pitch;
            m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
            m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
        }

        void NoClipControllerComponent::SetFov(float fov)
        {
            m_currentFov = GetClamp(fov, MinFov, MaxFov);
        }

        float NoClipControllerComponent::GetMouseSensitivityX()
        {
            return m_properties.m_mouseSensitivityX;
        }

        float NoClipControllerComponent::GetMouseSensitivityY()
        {
            return m_properties.m_mouseSensitivityY;
        }

        float NoClipControllerComponent::GetMoveSpeed()
        {
            return m_properties.m_moveSpeed;
        }

        float NoClipControllerComponent::GetPanningSpeed()
        {
            return m_properties.m_panningSpeed;
        }

        float NoClipControllerComponent::GetTouchSensitivity()
        {
            return m_properties.m_touchSensitivity;
        }

        NoClipControllerProperties NoClipControllerComponent::GetControllerProperties()
        {
            return m_properties;
        }

        AZ::Vector3 NoClipControllerComponent::GetPosition()
        {
            AZ::Vector3 position;
            AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
            return position;
        }

        float NoClipControllerComponent::GetHeading()
        {
            return m_currentHeading;
        }

        float NoClipControllerComponent::GetPitch()
        {
            return m_currentPitch;
        }

        float NoClipControllerComponent::GetFov()
        {
            return m_currentFov;
        }

    } // namespace Debug
} // namespace AZ

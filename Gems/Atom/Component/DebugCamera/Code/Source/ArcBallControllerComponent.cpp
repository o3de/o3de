/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>

#include <AzCore/Component/TransformBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <DebugCameraUtils.h>

namespace AZ
{
    namespace Debug
    {
        ArcBallControllerComponent::ArcBallControllerComponent()
            : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDefault())
        {}

        void ArcBallControllerComponent::Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<ArcBallControllerComponent, CameraControllerComponent, AZ::Component>()
                    ->Version(1);
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                behaviorContext->EBus<ArcBallControllerRequestBus>("ArcBallControllerRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Camera")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("SetCenter", &ArcBallControllerRequestBus::Events::SetCenter)
                    ->Event("SetPan", &ArcBallControllerRequestBus::Events::SetPan)
                    ->Event("SetDistance", &ArcBallControllerRequestBus::Events::SetDistance)
                    ->Event("SetMinDistance", &ArcBallControllerRequestBus::Events::SetMinDistance)
                    ->Event("SetMaxDistance", &ArcBallControllerRequestBus::Events::SetMaxDistance)
                    ->Event("SetHeading", &ArcBallControllerRequestBus::Events::SetHeading)
                    ->Event("SetPitch", &ArcBallControllerRequestBus::Events::SetPitch)
                    ->Event("SetPanningSensitivity", &ArcBallControllerRequestBus::Events::SetPanningSensitivity)
                    ->Event("SetZoomingSensitivity", &ArcBallControllerRequestBus::Events::SetZoomingSensitivity)
                    ->Event("GetCenter", &ArcBallControllerRequestBus::Events::GetCenter)
                    ->Event("GetPan", &ArcBallControllerRequestBus::Events::GetPan)
                    ->Event("GetDistance", &ArcBallControllerRequestBus::Events::GetDistance)
                    ->Event("GetMinDistance", &ArcBallControllerRequestBus::Events::GetMinDistance)
                    ->Event("GetMaxDistance", &ArcBallControllerRequestBus::Events::GetMaxDistance)
                    ->Event("GetHeading", &ArcBallControllerRequestBus::Events::GetHeading)
                    ->Event("GetPitch", &ArcBallControllerRequestBus::Events::GetPitch)
                    ->Event("GetPanningSensitivity", &ArcBallControllerRequestBus::Events::GetPanningSensitivity)
                    ->Event("GetZoomingSensitivity", &ArcBallControllerRequestBus::Events::GetZoomingSensitivity)
                    ;
            }
        }
        
        void ArcBallControllerComponent::OnEnabled()
        {
            // Reset parameters with initial values
            m_arcballActive = false;
            m_panningActive = false;
            m_center = AZ::Vector3::CreateZero();
            m_panningOffset = AZ::Vector3::CreateZero();
            m_panningOffsetDelta = AZ::Vector3::CreateZero();
            m_distance = 5.0f;
            m_minDistance = 0.1f;
            m_maxDistance = 10.0f;
            m_currentHeading = 0.0f;
            m_currentPitch = 0.0f;
            m_panningSensitivity = 1.0f;
            m_zoomingSensitivity = 1.0f;

            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            AzFramework::WindowSize windowSize;
            AzFramework::WindowRequestBus::EventResult(
                windowSize,
                windowHandle,
                &AzFramework::WindowRequestBus::Events::GetRenderResolution);

            m_windowWidth = windowSize.m_width;
            m_windowHeight = windowSize.m_height;

            ArcBallControllerRequestBus::Handler::BusConnect(GetEntityId());
            AzFramework::InputChannelEventListener::Connect();
            AZ::TickBus::Handler::BusConnect();
        }

        void ArcBallControllerComponent::OnDisabled()
        {
            TickBus::Handler::BusDisconnect();
            AzFramework::InputChannelEventListener::Disconnect();
            ArcBallControllerRequestBus::Handler::BusDisconnect();
        }
        
        void ArcBallControllerComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            AZ_UNUSED(time);

            if (m_distance < m_minDistance)
            {
                m_distance = m_minDistance;
            }
            else if (m_distance > m_maxDistance)
            {
                m_distance = m_maxDistance;
            }

            // The coordinate system is right-handed and Z-up. So heading is a rotation around the Z axis.
            // After that rotation we rotate around the (rotated by heading) X axis for pitch.
            AZ::Quaternion orientation = AZ::Quaternion::CreateRotationZ(m_currentHeading)
                * AZ::Quaternion::CreateRotationX(m_currentPitch);

            m_panningOffsetDelta *= deltaTime;
            m_panningOffset += (orientation.TransformVector(m_panningOffsetDelta));
            AZ::Vector3 position = (m_center + m_panningOffset) + (orientation.TransformVector(AZ::Vector3(0, -m_distance, 0)));

            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(orientation, position);
            AZ::TransformBus::Event(
                GetEntityId(), &AZ::TransformBus::Events::SetLocalTM, transform);
        }

        bool ArcBallControllerComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
        {
            static const float PixelToDegree = 1.0 / 360.0f;

            uint32_t handledChannels = ArcBallControllerChannel_None;

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            switch (inputChannel.GetState())
            {
            case AzFramework::InputChannel::State::Began:
            case AzFramework::InputChannel::State::Updated: // update the camera rotation
            {
                //Keyboard & Mouse
                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    m_arcballActive = true;
                    handledChannels |= ArcBallControllerChannel_Orientation;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    m_panningActive = true;
                    handledChannels |= ArcBallControllerChannel_Pan;
                }

                if (m_arcballActive)
                {
                    if (inputChannelId == AzFramework::InputDeviceMouse::Movement::X)
                    {
                        // modify yaw angle
                        m_currentHeading -= inputChannel.GetValue() * PixelToDegree;
                        m_currentHeading = NormalizeAngle(m_currentHeading);
                    }
                    else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Y)
                    {
                        // modify pitch angle
                        m_currentPitch -= inputChannel.GetValue() * PixelToDegree;
                        m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                        m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
                    }
                }
                else if (m_panningActive)
                {
                    if (inputChannelId == AzFramework::InputDeviceMouse::Movement::X)
                    {
                        m_panningOffsetDelta.SetX(-inputChannel.GetValue() * m_panningSensitivity);
                    }
                    else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Y)
                    {
                        m_panningOffsetDelta.SetZ(inputChannel.GetValue() * m_panningSensitivity);
                    }
                }

                if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    const float MouseWheelDeltaScale = 1.0f / 120.0f; // based on WHEEL_DELTA in WinUser.h
                    m_distance -= inputChannel.GetValue() * MouseWheelDeltaScale * m_zoomingSensitivity;
                    m_zoomingActive = true;
                    handledChannels |= ArcBallControllerChannel_Distance;
                }

                // Gamepad
                if (inputChannelId == AzFramework::InputDeviceGamepad::Trigger::L2)
                {
                    m_arcballActive = true;
                    handledChannels |= ArcBallControllerChannel_Orientation;
                }
                else if (inputChannelId == AzFramework::InputDeviceGamepad::Button::L1)
                {
                    m_panningActive = true;
                    handledChannels |= ArcBallControllerChannel_Pan;
                }

                if (m_arcballActive)
                {
                    if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RX)
                    {
                        // modify yaw angle
                        m_currentHeading -= inputChannel.GetValue() * PixelToDegree;
                        m_currentHeading = NormalizeAngle(m_currentHeading);
                    }
                    else if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RY)
                    {
                        // modify pitch angle
                        m_currentPitch += inputChannel.GetValue() * PixelToDegree;
                        m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                        m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
                    }
                }
                else if (m_panningActive)
                {
                    if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RX)
                    {
                        m_panningOffsetDelta.SetX(-inputChannel.GetValue() * 10.0f * m_panningSensitivity);
                    }
                    else if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::RY)
                    {
                        m_panningOffsetDelta.SetZ(inputChannel.GetValue() * 10.0f * m_panningSensitivity);
                    }
                }

                if (inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::LY)
                {
                    m_distance -= inputChannel.GetValue() * m_zoomingSensitivity;
                    m_zoomingActive = true;
                    handledChannels |= ArcBallControllerChannel_Distance;
                }

                // Touch controls works depending which side of the screen you start the touch event.
                // Left side controls the heading and pitch. Right side controls the panning.
                // Only one touch control can be active at the same time.
                if (inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    auto* positionData = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ::Vector2 screenPos = positionData->m_normalizedPosition;
                    auto deltaInPixels = screenPos - m_lastTouchPosition;
                    deltaInPixels *= AZ::Vector2(static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight));
                    if (inputChannel.GetState() == AzFramework::InputChannel::State::Began)
                    {
                        m_panningActive = screenPos.GetX() > 0.5f ? true : false;
                        m_arcballActive = !m_panningActive;
                    }
                    else if(m_panningActive)
                    {
                        m_panningOffsetDelta.SetX(-deltaInPixels.GetX() * m_panningSensitivity);
                        m_panningOffsetDelta.SetZ(deltaInPixels.GetY() * m_panningSensitivity);
                    }
                    else if(m_arcballActive)
                    {
                        // modify yaw angle
                        m_currentHeading -= deltaInPixels.GetX() * PixelToDegree;
                        m_currentHeading = NormalizeAngle(m_currentHeading);

                        // modify pitch angle
                        m_currentPitch -= deltaInPixels.GetY() * PixelToDegree;
                        m_currentPitch = AZStd::max(m_currentPitch, -AZ::Constants::HalfPi);
                        m_currentPitch = AZStd::min(m_currentPitch, AZ::Constants::HalfPi);
                    }

                    m_lastTouchPosition = screenPos;

                    if (m_panningActive)
                    {
                        handledChannels |= ArcBallControllerChannel_Pan;
                    }
                    else if (m_arcballActive)
                    {
                        handledChannels |= ArcBallControllerChannel_Orientation;
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
                uint32_t handledChannels2 = ArcBallControllerChannel_None;

                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    m_arcballActive = false;
                    handledChannels2 |= ArcBallControllerChannel_Orientation;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    m_panningActive = false;
                    handledChannels2 |= ArcBallControllerChannel_Pan;
                }
                else if (inputChannelId == AzFramework::InputDeviceGamepad::Trigger::L2)
                {
                    m_arcballActive = false;
                    handledChannels2 |= ArcBallControllerChannel_Orientation;
                }
                else if (inputChannelId == AzFramework::InputDeviceGamepad::Button::L1)
                {
                    m_panningActive = false;
                    handledChannels2 |= ArcBallControllerChannel_Pan;
                }
                else if (inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    if (m_panningActive)
                    {
                        handledChannels2 |= ArcBallControllerChannel_Pan;
                    }
                    else if (m_arcballActive)
                    {
                        handledChannels2 |= ArcBallControllerChannel_Orientation;
                    }

                    m_panningActive = false;
                    m_arcballActive = false;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z ||
                    inputChannelId == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::LY)
                {
                    m_zoomingActive = false;
                    handledChannels2 |= ArcBallControllerChannel_Distance;
                }

                if (handledChannels2)
                {
                    CameraControllerNotificationBus::Broadcast(&CameraControllerNotifications::OnCameraMoveEnded, RTTI_GetType(), handledChannels2);
                }
            }
            default:
            {
                break;
            }
            }
            return false;
        }

        void ArcBallControllerComponent::SetCenter(AZ::Vector3 center)
        {
            m_center = center;
        }

        void ArcBallControllerComponent::SetPan(AZ::Vector3 pan)
        {
            m_panningOffset = pan;
        }

        void ArcBallControllerComponent::SetDistance(float distance)
        {
            m_distance = distance;
        }

        void ArcBallControllerComponent::SetMinDistance(float minDistance)
        {
            m_minDistance = minDistance;
            m_distance = AZ::GetMax(m_distance, m_minDistance);
        }

        void ArcBallControllerComponent::SetMaxDistance(float maxDistance)
        {
            m_maxDistance = maxDistance;
            m_distance = AZ::GetMin(m_distance, m_maxDistance);
        }

        void ArcBallControllerComponent::SetHeading(float heading)
        {
            m_currentHeading = heading;
        }

        void ArcBallControllerComponent::SetPitch(float pitch)
        {
            m_currentPitch = pitch;
        }

        void ArcBallControllerComponent::SetPanningSensitivity(float panningSensitivity)
        {
            m_panningSensitivity = AZ::GetMax(panningSensitivity, 0.0f);
        }

        void ArcBallControllerComponent::SetZoomingSensitivity(float zoomingSensitivity)
        {
            m_zoomingSensitivity = AZ::GetMax(zoomingSensitivity, 0.0f);
        }

        AZ::Vector3 ArcBallControllerComponent::GetCenter()
        {
            return m_center;
        }

        AZ::Vector3 ArcBallControllerComponent::GetPan()
        {
            return m_panningOffset;
        }

        float ArcBallControllerComponent::GetDistance()
        {
            return m_distance;
        }

        float ArcBallControllerComponent::GetMinDistance()
        {
            return m_minDistance;
        }

        float ArcBallControllerComponent::GetMaxDistance()
        {
            return m_maxDistance;
        }

        float ArcBallControllerComponent::GetHeading()
        {
            return m_currentHeading;
        }

        float ArcBallControllerComponent::GetPitch()
        {
            return m_currentPitch;
        }

        float ArcBallControllerComponent::GetPanningSensitivity()
        {
            return m_panningSensitivity;
        }

        float ArcBallControllerComponent::GetZoomingSensitivity()
        {
            return m_zoomingSensitivity;
        }
    } // namespace Debug
} // namespace AZ

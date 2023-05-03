/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Component/DebugCamera/CameraControllerComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Math/Transform.h>

#include <AzCore/std/containers/bitset.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>

namespace AZ
{
    namespace Debug
    {
        class NoClipControllerComponent
            : public CameraControllerComponent
            , public TickBus::Handler
            , public AzFramework::InputChannelEventListener
            , public NoClipControllerRequestBus::Handler
        {
        public:
            AZ_COMPONENT(NoClipControllerComponent, "{FDDF608A-7866-4886-87E5-6F02899C6C4D}", CameraControllerComponent);

            NoClipControllerComponent();
            virtual ~NoClipControllerComponent() = default;

            static void Reflect(AZ::ReflectContext* reflection);
            
        private:
            // CameraControllerComponent overrides
            void OnEnabled() override;
            void OnDisabled() override;

            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // AzFramework::InputChannelEventListener
            bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

            // ControllerRequestsBus::Handler
            void SetMouseSensitivityX(float mouseSensitivityX) override;
            void SetMouseSensitivityY(float mouseSensitivityY) override;
            void SetMoveSpeed(float moveSpeed) override;
            void SetPanningSpeed(float panningSpeed) override;
            void SetControllerProperties(const NoClipControllerProperties& properties) override;
            void SetTouchSensitivity(float touchSensitivity) override;

            void SetPosition(AZ::Vector3 position) override;
            void SetHeading(float heading) override;
            void SetPitch(float pitch) override;
            void SetFov(float fov) override;

            void SetCameraStateForward(float value) override;
            void SetCameraStateBack(float value) override;
            void SetCameraStateLeft(float value) override;
            void SetCameraStateRight(float value) override;
            void SetCameraStateUp(float value) override;
            void SetCameraStateDown(float value) override;

            float GetMouseSensitivityX() override;
            float GetMouseSensitivityY() override;
            float GetMoveSpeed() override;
            float GetPanningSpeed() override;
            float GetTouchSensitivity() override;
            NoClipControllerProperties GetControllerProperties() override;

            AZ::Vector3 GetPosition() override;
            float GetHeading() override;
            float GetPitch() override;
            float GetFov() override;

            enum CameraKeys
            {
                Forward = 0,
                Back,
                Left,
                Right,
                Up,
                Down,

                FastMode,

                Count
            };

            bool m_mouseLookEnabled = false;
            AZStd::bitset<static_cast<size_t>(CameraKeys::Count)> m_inputStates = 0;

            NoClipControllerProperties m_properties;

            float m_currentHeading = 0.0f;
            float m_currentPitch = 0.0f;
            float m_currentFov = 0.0f;

            float m_lastForward = 0.0f;
            float m_lastStrafe = 0.0f;
            float m_lastAscent = 0.0f;

            struct TouchEvent
            {
                static const AzFramework::InputChannelId InvalidTouchChannelId;
                AZ::Vector2 m_initialPos = AZ::Vector2::CreateZero();
                AzFramework::InputChannelId m_channelId = InvalidTouchChannelId;
            };

            TouchEvent m_mouseLookTouch;
            TouchEvent m_movementTouch;
        };
    } // namespace Debug
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <Cry_Math.h>
#include <AtomBridge/FlyCameraInputBus.h>

namespace AZ
{
    namespace AtomBridge
    {
        /// This is based on the FlyCameraInputComponent in SamplesProject and is just used to test the CameraComponent
        class FlyCameraInputComponent
            : public AZ::Component
            , public AZ::TickBus::Handler
            , public AzFramework::InputChannelEventListener
            , public FlyCameraInputBus::Handler
        {
        public:
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void Reflect(AZ::ReflectContext* reflection);

            AZ_COMPONENT(FlyCameraInputComponent, "{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}");
            virtual ~FlyCameraInputComponent();

            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // AZ::TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // AzFramework::InputChannelEventListener
            bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

            // FlyCameraInputInterface
            void SetIsEnabled(bool isEnabled) override;
            bool GetIsEnabled() override;

        private:
            void OnMouseEvent(const AzFramework::InputChannel& inputChannel);
            void OnKeyboardEvent(const AzFramework::InputChannel& inputChannel);
            void OnGamepadEvent(const AzFramework::InputChannel& inputChannel);
            void OnTouchEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);
            void OnVirtualLeftThumbstickEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);
            void OnVirtualRightThumbstickEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);

            float GetViewWidth() const;
            float GetViewHeight() const;

            static const AZ::Crc32 UnknownInputChannelId;

            // Editable Properties
            float m_moveSpeed = 20.0f;
            float m_rotationSpeed = 5.0f;

            float m_mouseSensitivity = 0.025f;
            float m_virtualThumbstickRadiusAsPercentageOfScreenWidth = 0.1f;

            bool m_InvertRotationInputAxisX = false;
            bool m_InvertRotationInputAxisY = false;

            bool m_isEnabled = true;

            // Run-time Properties
            Vec3 m_movement = ZERO;
            Vec2 m_rotation = ZERO;

            Vec2 m_leftDownPosition = ZERO;
            AZ::Crc32 m_leftFingerId = UnknownInputChannelId;

            Vec2 m_rightDownPosition = ZERO;
            AZ::Crc32 m_rightFingerId = UnknownInputChannelId;

            int m_thumbstickTextureId = 0;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Math/Transform.h>

#include <AzCore/std/containers/bitset.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <Atom/Component/DebugCamera/ArcBallControllerBus.h>
#include <Atom/Component/DebugCamera/CameraControllerComponent.h>

namespace AZ
{
    namespace Debug
    {
        class ArcBallControllerComponent
            : public CameraControllerComponent
            , public TickBus::Handler
            , public AzFramework::InputChannelEventListener
            , ArcBallControllerRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ArcBallControllerComponent, "{3CCDE644-2798-4A58-992C-1C420599FCEE}", CameraControllerComponent);

            ArcBallControllerComponent();
            virtual ~ArcBallControllerComponent() = default;

            static void Reflect(AZ::ReflectContext* reflection);
            
        private:
            /// CameraControllerComponent
            void OnEnabled() override;
            void OnDisabled() override;

            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // AzFramework::InputChannelEventListener
            bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

            void SetCenter(AZ::Vector3 center) override;
            void SetPan(AZ::Vector3 pan) override;
            void SetDistance(float distance) override;
            void SetMinDistance(float minDistance) override;
            void SetMaxDistance(float maxDistance) override;
            void SetHeading(float heading) override;
            void SetPitch(float pitch) override;
            void SetPanningSensitivity(float panningSensitivity) override;
            void SetZoomingSensitivity(float zoomingSensitivity) override;

            AZ::Vector3 GetCenter() override;
            AZ::Vector3 GetPan() override;
            float GetDistance() override;
            float GetMinDistance() override;
            float GetMaxDistance() override;
            float GetHeading() override;
            float GetPitch() override;
            float GetPanningSensitivity() override;
            float GetZoomingSensitivity() override;

            bool m_arcballActive = false;
            bool m_panningActive = false;
            bool m_zoomingActive = false;
            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            AZ::Vector3 m_panningOffset = AZ::Vector3::CreateZero();
            AZ::Vector3 m_panningOffsetDelta = AZ::Vector3::CreateZero();
            float m_distance = 5.0f;
            float m_minDistance = 0.1f;
            float m_maxDistance = 10.0f;
            float m_currentHeading = 0.0f;
            float m_currentPitch = 0.0f;
            float m_panningSensitivity = 1.0f;
            float m_zoomingSensitivity = 1.0f;

            AZ::Vector2 m_lastTouchPosition;
            uint32_t m_windowWidth = 0;
            uint32_t m_windowHeight = 0;
        };
    } // namespace Debug
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "IGestureRecognizer.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Time/ITime.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerPinch : public RecognizerContinuous
    {
    public:
        static float GetDefaultMinPixelsMoved() { return 50.0f; }
        static float GetDefaultMaxAngleDegrees() { return 15.0f; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);
            AZ_RTTI(Config, "{DD3CAAB0-4D81-4CCD-87E3-3AB120B080C6}");
            static void Reflect(AZ::ReflectContext* context);

            Config()
                : minPixelsMoved(GetDefaultMinPixelsMoved())
                , maxAngleDegrees(GetDefaultMaxAngleDegrees())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            float minPixelsMoved;
            float maxAngleDegrees;
            int32_t priority;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerPinch, AZ::SystemAllocator, 0);
        AZ_RTTI(RecognizerPinch, "{C44DE7E3-1DBE-48CA-BD60-AD2633E11137}", RecognizerContinuous);

        explicit RecognizerPinch(const Config& config = GetDefaultConfig());
        ~RecognizerPinch() override;

        int32_t GetPriority() const override { return m_config.priority; }
        bool OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;

        Config& GetConfig() { return m_config; }
        const Config& GetConfig() const { return m_config; }
        void SetConfig(const Config& config) { m_config = config; }

        AZ::Vector2 GetStartPosition0() const { return m_startPositions[0]; }
        AZ::Vector2 GetStartPosition1() const { return m_startPositions[1]; }

        AZ::Vector2 GetCurrentPosition0() const { return m_currentPositions[0]; }
        AZ::Vector2 GetCurrentPosition1() const { return m_currentPositions[1]; }

        AZ::Vector2 GetStartMidpoint() const { return Lerp(GetStartPosition0(), GetStartPosition1(), 0.5f); }
        AZ::Vector2 GetCurrentMidpoint() const { return Lerp(GetCurrentPosition0(), GetCurrentPosition1(), 0.5f); }

        float GetStartDistance() const { return GetStartPosition1().GetDistance(GetStartPosition0()); }
        float GetCurrentDistance() const { return GetCurrentPosition1().GetDistance(GetCurrentPosition0()); }
        float GetPinchRatio() const
        {
            const float startDistance = GetStartDistance();
            return startDistance ? GetCurrentDistance() / startDistance : 0.0f;
        }

    private:
        enum class State
        {
            Idle = -1,
            Pressed0 = 0,
            Pressed1 = 1,
            PressedBoth,
            Pinching
        };

        static const uint32_t s_maxPinchPointerIndex = 1;

        Config m_config;

        ScreenPosition m_startPositions[2];
        ScreenPosition m_currentPositions[2];

        AZ::TimeMs m_lastUpdateTimes[2];

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerPinch.inl>

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
    class RecognizerSwipe : public RecognizerDiscrete
    {
    public:
       static float GetDefaultMaxSecondsHeld() { return 0.5f; }
       static float GetDefaultMinPixelsMoved() { return 100.0f; }
       static uint32_t GetDefaultPointerIndex() { return 0; }
       static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);
            AZ_RTTI(Config, "{60CC943E-9973-4046-B0AE-32A5B8B5F7A5}");
            static void Reflect(AZ::ReflectContext* context);

           Config()
                : maxSecondsHeld(GetDefaultMaxSecondsHeld())
                , minPixelsMoved(GetDefaultMinPixelsMoved())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            float maxSecondsHeld;
            float minPixelsMoved;
            uint32_t pointerIndex;
            int32_t priority;
        };
       static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerSwipe, AZ::SystemAllocator, 0);
        AZ_RTTI(RecognizerSwipe, "{3030E923-531F-4CE6-BC8E-84238FA47AB9}", RecognizerDiscrete);

       explicit RecognizerSwipe(const Config& config = GetDefaultConfig());
       ~RecognizerSwipe() override;

       int32_t GetPriority() const override { return m_config.priority; }
       bool OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
       bool OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
       bool OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;

       Config& GetConfig() { return m_config; }
       const Config& GetConfig() const { return m_config; }
       void SetConfig(const Config& config) { m_config = config; }

       AZ::Vector2 GetStartPosition() const { return m_startPosition; }
       AZ::Vector2 GetEndPosition() const { return m_endPosition; }

       AZ::Vector2 GetDelta() const { return GetEndPosition() - GetStartPosition(); }
       AZ::Vector2 GetDirection() const { return GetDelta().GetNormalized(); }

       float GetDistance() const { return GetEndPosition().GetDistance(GetStartPosition()); }
       float GetDuration() const { return AZ::TimeMsToSeconds(m_endTime - m_startTime); }
       float GetVelocity() const { return GetDistance() / GetDuration(); }

    private:
        enum class State
        {
            Idle,
            Pressed
        };

        Config m_config;

        ScreenPosition m_startPosition;
        ScreenPosition m_endPosition;

        AZ::TimeMs m_startTime;
        AZ::TimeMs m_endTime;

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerSwipe.inl>

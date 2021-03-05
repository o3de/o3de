/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include "IGestureRecognizer.h"

#include <AzCore/RTTI/ReflectContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerClickOrTap : public RecognizerDiscrete
    {
    public:
        static float GetDefaultMaxSecondsHeld() { return 0.5f; }
        static float GetDefaultMaxPixelsMoved() { return 20.0f; }
        static float GetDefaultMaxSecondsBetweenClicksOrTaps() { return 0.5f; }
        static float GetDefaultMaxPixelsBetweenClicksOrTaps() { return 100.0f; }
        static uint32_t GetDefaultMinClicksOrTaps() { return 1; }
        static uint32_t GetDefaultPointerIndex() { return 0; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);
            AZ_RTTI(Config, "{E1B99E50-605A-467E-B26E-B9F72A98A04F}");
            static void Reflect(AZ::ReflectContext* context);

            Config()
                : maxSecondsHeld(GetDefaultMaxSecondsHeld())
                , maxPixelsMoved(GetDefaultMaxPixelsMoved())
                , maxSecondsBetweenClicksOrTaps(GetDefaultMaxSecondsBetweenClicksOrTaps())
                , maxPixelsBetweenClicksOrTaps(GetDefaultMaxPixelsBetweenClicksOrTaps())
                , minClicksOrTaps(GetDefaultMinClicksOrTaps())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            bool IsMultiClickOrTap() const { return minClicksOrTaps > 1; }

            float maxSecondsHeld;
            float maxPixelsMoved;
            float maxSecondsBetweenClicksOrTaps;
            float maxPixelsBetweenClicksOrTaps;
            uint32_t minClicksOrTaps;
            uint32_t pointerIndex;
            int32_t priority;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerClickOrTap, AZ::SystemAllocator, 0);
        AZ_RTTI(RecognizerClickOrTap, "{C401A49C-6D88-4268-8E2D-6BAECFD7146E}", RecognizerDiscrete);

        explicit RecognizerClickOrTap(const Config& config = GetDefaultConfig());
        ~RecognizerClickOrTap() override;

        int32_t GetPriority() const override { return m_config.priority; }
        bool OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;

        Config& GetConfig() { return m_config; }
        const Config& GetConfig() const { return m_config; }
        void SetConfig(const Config& config) { m_config = config; }

        AZ::Vector2 GetStartPosition() const { return m_positionOfFirstEvent; }
        AZ::Vector2 GetEndPosition() const { return m_positionOfLastEvent; }

    private:
        enum class State
        {
            Idle,
            Pressed,
            Released
        };

        Config m_config;

        int64 m_timeOfLastEvent;
        ScreenPosition m_positionOfFirstEvent;
        ScreenPosition m_positionOfLastEvent;

        uint32_t m_currentCount;
        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerClickOrTap.inl>

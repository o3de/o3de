/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "IGestureRecognizer.h"

#include <CryCommon/ISystem.h>
#include <CryCommon/ITimer.h>
#include <AzCore/RTTI/ReflectContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerHold : public RecognizerContinuous
    {
    public:
        static float GetDefaultMinSecondsHeld() { return 2.0f; }
        static float GetDefaultMaxPixelsMoved() { return 20.0f; }
        static uint32_t GetDefaultPointerIndex() { return 0; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);
            AZ_RTTI(Config, "{3D854AD1-73C0-4E26-A609-F20FC04F78F3}");
            static void Reflect(AZ::ReflectContext* context);

            Config()
                : minSecondsHeld(GetDefaultMinSecondsHeld())
                , maxPixelsMoved(GetDefaultMaxPixelsMoved())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            float minSecondsHeld;
            float maxPixelsMoved;
            uint32_t pointerIndex;
            int32_t priority;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerHold, AZ::SystemAllocator, 0);
        AZ_RTTI(RecognizerHold, "{7FC9AB8D-0A94-40A6-8FE0-84C752D786DC}", RecognizerContinuous);

        explicit RecognizerHold(const Config& config = GetDefaultConfig());
        ~RecognizerHold() override;

        int32_t GetPriority() const override { return m_config.priority; }
        bool OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;

        Config& GetConfig() { return m_config; }
        const Config& GetConfig() const { return m_config; }
        void SetConfig(const Config& config) { m_config = config; }

        AZ::Vector2 GetStartPosition() const { return m_startPosition; }
        AZ::Vector2 GetCurrentPosition() const { return m_currentPosition; }

        float GetDuration() const { return (gEnv && gEnv->pTimer) ? gEnv->pTimer->GetFrameStartTime().GetDifferenceInSeconds(m_startTime) : 0.0f; }

    private:
        enum class State
        {
            Idle,
            Pressed,
            Held
        };

        Config m_config;

        int64 m_startTime;
        ScreenPosition m_startPosition;
        ScreenPosition m_currentPosition;

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerHold.inl>

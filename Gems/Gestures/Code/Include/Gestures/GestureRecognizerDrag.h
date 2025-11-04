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
    class RecognizerDrag : public RecognizerContinuous
    {
    public:
        static float GetDefaultMinSecondsHeld() { return 0.0f; }
        static float GetDefaultMinPixelsMoved() { return 20.0f; }
        static uint32_t GetDefaultPointerIndex() { return 0; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator);
            AZ_RTTI(Config, "{F28051E1-8B39-40BC-B80E-0CBAF1EF288A}");
            static void Reflect(AZ::ReflectContext* context);

            Config()
                : minSecondsHeld(GetDefaultMinSecondsHeld())
                , minPixelsMoved(GetDefaultMinPixelsMoved())
                , pointerIndex(GetDefaultPointerIndex())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            float minSecondsHeld;
            float minPixelsMoved;
            uint32_t pointerIndex;
            int32_t priority;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerDrag, AZ::SystemAllocator);
        AZ_RTTI(RecognizerDrag, "{B244C54C-1F5C-420E-8F47-025AFEB7A499}", RecognizerContinuous);

        explicit RecognizerDrag(const Config& config = GetDefaultConfig());
        ~RecognizerDrag() override;

        int32_t GetPriority() const override { return m_config.priority; }
        bool OnPressedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnDownEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;
        bool OnReleasedEvent(const AZ::Vector2& screenPosition, uint32_t pointerIndex) override;

        Config& GetConfig() { return m_config; }
        const Config& GetConfig() const { return m_config; }
        void SetConfig(const Config& config) { m_config = config; }

        AZ::Vector2 GetStartPosition() const { return m_startPosition; }
        AZ::Vector2 GetCurrentPosition() const { return m_currentPosition; }

        AZ::Vector2 GetDelta() const { return GetCurrentPosition() - GetStartPosition(); }
        float GetDistance() const { return GetCurrentPosition().GetDistance(GetStartPosition()); }

    private:
        enum class State
        {
            Idle,
            Pressed,
            Dragging
        };

        Config m_config;

        AZ::TimeMs m_startTime;
        ScreenPosition m_startPosition;
        ScreenPosition m_currentPosition;

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerDrag.inl>

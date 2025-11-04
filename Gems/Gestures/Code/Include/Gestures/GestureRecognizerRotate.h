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
#include <AzCore/Math/MathUtils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerRotate : public RecognizerContinuous
    {
    public:
        static float GetDefaultMaxPixelsMoved() { return 50.0f; }
        static float GetDefaultMinAngleDegrees() { return 15.0f; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator);
            AZ_RTTI(Config, "{B329235B-3C8E-4554-8751-9DBCFC61312E}");
            static void Reflect(AZ::ReflectContext* context);

            Config()
                : maxPixelsMoved(GetDefaultMaxPixelsMoved())
                , minAngleDegrees(GetDefaultMinAngleDegrees())
                , priority(GetDefaultPriority())
            {}
            virtual ~Config() = default;

            float maxPixelsMoved;
            float minAngleDegrees;
            int32_t priority;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerRotate, AZ::SystemAllocator);
        AZ_RTTI(RecognizerRotate, "{ABD687F0-FEFA-4424-81CA-4AC3773620D9}", RecognizerContinuous);

        explicit RecognizerRotate(const Config& config = GetDefaultConfig());
         ~RecognizerRotate() override;

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

       AZ::Vector2 GetStartMidpoint() const { return AZ::Lerp(GetStartPosition0(), GetStartPosition1(), 0.5f); }
       AZ::Vector2 GetCurrentMidpoint() const { return AZ::Lerp(GetCurrentPosition0(), GetCurrentPosition1(), 0.5f); }

       float GetStartDistance() const { return GetStartPosition1().GetDistance(GetStartPosition0()); }
       float GetCurrentDistance() const { return GetCurrentPosition1().GetDistance(GetCurrentPosition0()); }
       float GetSignedRotationInDegrees() const;

    private:
        enum class State
        {
            Idle = -1,
            Pressed0 = 0,
            Pressed1 = 1,
            PressedBoth,
            Rotating
        };

        static const uint32_t s_maxRotatePointerIndex = 1;

        Config m_config;

        ScreenPosition m_startPositions[2];
        ScreenPosition m_currentPositions[2];

        AZ::TimeMs m_lastUpdateTimes[2];

        State m_currentState;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerRotate.inl>

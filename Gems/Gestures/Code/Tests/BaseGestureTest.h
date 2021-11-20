/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzTest/AzTest.h>
#include <Gestures/IGestureRecognizer.h>

namespace GesturesTests
{
    struct StubTimer : public AZ::StubTimeSystem
    {
        AZ::TimeMs GetRealElapsedTimeMs() const override
        {
            return m_realElapsedTime;
        }

        AZ::TimeMs m_realElapsedTime = AZ::Time::ZeroTimeMs;
    };
} // namespace GesturesTests

class BaseGestureTest : public ::testing::Test
{
public:
    BaseGestureTest()
        : m_pos(0.0f, 0.0f)
    {
    }

    void SetUp() override
    {
        // global environment stubs
        m_env = new(AZ_OS_MALLOC(sizeof(SSystemGlobalEnvironment), alignof(SSystemGlobalEnvironment))) SSystemGlobalEnvironment();
        gEnv = m_env;
        m_stubTimer = new GesturesTests::StubTimer();
        // simulated position
        m_pos = AZ::Vector2(0.0f, 0.0f);
    }

    void TearDown() override
    {
        gEnv = nullptr;
        if (m_env)
        {
            m_env->~SSystemGlobalEnvironment();
            AZ_OS_FREE(m_env);
            m_env = nullptr;
        }
        delete m_stubTimer;
    }

protected:
    // time manipulation

    void SetTime(float sec)
    {
        m_stubTimer->m_realElapsedTime = AZ::SecondsToTimeMs(sec);
    }

    // simple position caching interface

    void MoveTo(float x, float y)
    {
        m_pos = AZ::Vector2(x, y);
    }

    void MouseDownAt(Gestures::IRecognizer& recognizer, float sec)
    {
        Press(recognizer, 0, m_pos, sec);
    }

    void MouseUpAt(Gestures::IRecognizer& recognizer, float sec)
    {
        Release(recognizer, 0, m_pos, sec);
    }

    // more direct interface

    void Press(Gestures::IRecognizer& recognizer, uint index, AZ::Vector2 pos, float sec)
    {
        SetTime(sec);
        recognizer.OnPressedEvent(pos, index);
    }

    void Move(Gestures::IRecognizer& recognizer, uint index, AZ::Vector2 pos, float sec)
    {
        SetTime(sec);
        recognizer.OnDownEvent(pos, index);
    }

    void Release(Gestures::IRecognizer& recognizer, uint index, AZ::Vector2 pos, float sec)
    {
        SetTime(sec);
        recognizer.OnReleasedEvent(pos, index);
    }

private:
    SSystemGlobalEnvironment* m_env = nullptr;
    GesturesTests::StubTimer* m_stubTimer = nullptr;
    AZ::Vector2 m_pos;
};

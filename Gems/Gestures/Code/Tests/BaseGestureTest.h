/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzTest/AzTest.h>
#include <Mocks/StubTimer.h>
#include <Gestures/IGestureRecognizer.h>
#include <AzCore/Memory/OSAllocator.h>

class BaseGestureTest
    : public ::testing::Test
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
        m_stubTimer = new StubTimer(1.0f / 30.0f);
        gEnv = m_env;
        gEnv->pTimer = m_stubTimer;

        // simulated position
        m_pos = AZ::Vector2(0.0f, 0.0f);
    }

    void TearDown() override
    {
        gEnv->pTimer = nullptr;
        gEnv = nullptr;
        if (m_stubTimer)
        {
            delete m_stubTimer;
            m_stubTimer = nullptr;
        }
        if (m_env)
        {
            m_env->~SSystemGlobalEnvironment();
            AZ_OS_FREE(m_env);
            m_env = nullptr;
        }
    }


protected:

    // time manipulation

    void SetTime(float sec)
    {
        m_stubTimer->SetTime(sec);
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
    SSystemGlobalEnvironment* m_env;
    StubTimer* m_stubTimer;
    AZ::Vector2 m_pos;
};



/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Gestures/GestureRecognizerPinch.h>
#include "BaseGestureTest.h"

namespace
{
    Gestures::RecognizerPinch::Config defaultConfig;
}


class PinchTests
    : public BaseGestureTest
{
public:
    void SetUp() override
    {
        BaseGestureTest::SetUp();
    }

    void TearDown() override
    {
        BaseGestureTest::TearDown();
    }
};

class MockPinchRecognizer
    : public Gestures::RecognizerPinch
{
public:
    MockPinchRecognizer()
        : m_initCount(0)
        , m_updateCount(0)
        , m_endCount(0)
    {
    }

    int m_initCount;
    int m_updateCount;
    int m_endCount;

    void OnContinuousGestureInitiated() override { ++m_initCount; }
    void OnContinuousGestureUpdated() override { ++m_updateCount; }
    void OnContinuousGestureEnded() override { ++m_endCount; }
};


TEST_F(PinchTests, Sanity_Pass)
{
    // Tests that Setup/TearDown work as expected
}

TEST_F(PinchTests, NoInput_DefaultConfig_NotRecognized)
{
    MockPinchRecognizer mockRecognizer;
    mockRecognizer.SetConfig(defaultConfig);
    ASSERT_EQ(0, mockRecognizer.m_initCount);
}

TEST_F(PinchTests, Touch_OneFinger_InitNotCalled)
{
    MockPinchRecognizer mockRecognizer;
    mockRecognizer.SetConfig(defaultConfig);

    Press(mockRecognizer, 0, AZ::Vector2(ZERO), 0.0f);

    ASSERT_EQ(0, mockRecognizer.m_initCount);
}

TEST_F(PinchTests, Touch_TwoFingersSlightlyApartNoMovement_InitNotCalled)
{
    MockPinchRecognizer mockRecognizer;
    mockRecognizer.SetConfig(defaultConfig);

    Press(mockRecognizer, 0, AZ::Vector2(ZERO), 0.0f);
    Press(mockRecognizer, 1, AZ::Vector2(0.5f), 0.0f);

    // both are down, but they haven't moved the "min pixels moved" distance
    // so the gesture has not been initialized
    ASSERT_EQ(0, mockRecognizer.m_initCount);
}

TEST_F(PinchTests, PinchOutward_GreaterThanMinDistance_InitCalled)
{
    MockPinchRecognizer mockRecognizer;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(config);

    Press(mockRecognizer, 0, AZ::Vector2(ZERO), 0.0f);
    Press(mockRecognizer, 1, AZ::Vector2(ZERO), 0.0f);
    Move(mockRecognizer, 0, AZ::Vector2(-5.01f, 0.0f), 1.0f);
    Move(mockRecognizer, 1, AZ::Vector2(5.01f, 0.0f), 1.0f);

    ASSERT_EQ(1, mockRecognizer.m_initCount);
}

TEST_F(PinchTests, PinchInward_GreaterThanMinDistance_InitCalled)
{
    MockPinchRecognizer mockRecognizer;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(config);

    Press(mockRecognizer, 0, AZ::Vector2(-15.01f, 0.0f), 0.0f);
    Press(mockRecognizer, 1, AZ::Vector2(15.01f, 0.0f), 0.0f);
    Move(mockRecognizer, 0, AZ::Vector2(-10.00f, 0.0f), 1.0f);
    Move(mockRecognizer, 1, AZ::Vector2(10.00f, 0.0f), 1.0f);

    ASSERT_EQ(1, mockRecognizer.m_initCount);
}

TEST_F(PinchTests, ReleaseBothTouches_AfterInitialized_EndedCalled)
{
    MockPinchRecognizer mockRecognizer;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(config);

    const AZ::Vector2 end(5.01f, 0.0f);
    Press(mockRecognizer, 0, AZ::Vector2(ZERO), 0.0f);
    Press(mockRecognizer, 1, AZ::Vector2(ZERO), 0.0f);
    Move(mockRecognizer, 0, -end, 1.0f);
    Move(mockRecognizer, 1, end, 1.0f);
    Release(mockRecognizer, 0, -end, 2.0f);
    Release(mockRecognizer, 1, end, 2.0f);

    ASSERT_EQ(1, mockRecognizer.m_initCount);
    ASSERT_EQ(1, mockRecognizer.m_endCount);
}

TEST_F(PinchTests, ReleaseOneTouch_AfterInitialized_EndedCalled)
{
    MockPinchRecognizer mockRecognizer;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(config);

    const AZ::Vector2 end(5.01f, 0.0f);
    Press(mockRecognizer, 0, AZ::Vector2(ZERO), 0.0f);
    Press(mockRecognizer, 1, AZ::Vector2(ZERO), 0.0f);
    Move(mockRecognizer, 0, -end, 1.0f);
    Move(mockRecognizer, 1, end, 1.0f);
    Release(mockRecognizer, 0, -end, 2.0f);
    //Release(mockRecognizer, 1, end, 2.0f);

    ASSERT_EQ(1, mockRecognizer.m_initCount);
    ASSERT_EQ(1, mockRecognizer.m_endCount);
}

TEST_F(PinchTests, ReleaseTouches_PinchNeverStarted_NoInitNoEnd)
{
    MockPinchRecognizer mockRecognizer;
    Gestures::RecognizerPinch::Config config;
    config.minPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(config);

    const AZ::Vector2 start(10.0f, 0.0f);
    const AZ::Vector2 end(9.0f, 0.0f); // not enough to initiate a pinch

    Press(mockRecognizer, 0, -start, 0.0f);
    Press(mockRecognizer, 1, start, 0.0f);
    Move(mockRecognizer, 0, -end, 1.0f);
    Move(mockRecognizer, 1, end, 1.0f);
    Release(mockRecognizer, 0, -end, 2.0f);
    Release(mockRecognizer, 1, end, 2.0f);

    ASSERT_EQ(0, mockRecognizer.m_initCount);
    ASSERT_EQ(0, mockRecognizer.m_endCount);
}

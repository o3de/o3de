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
#include "Gestures_precompiled.h"
#include <AzTest/AzTest.h>
#include <Mocks/StubTimer.h>
#include <Gestures/GestureRecognizerClickOrTap.h>
#include "BaseGestureTest.h"

namespace
{
    Gestures::RecognizerClickOrTap::Config singleTapOneSecond;
}

class MockRecognizer
    : public Gestures::RecognizerClickOrTap
{
public:
    MockRecognizer()
        : m_count(0)
    {
    }
    int m_count;

    void OnDiscreteGestureRecognized() override
    {
        ++m_count;
    }
};

class SimpleTests
    : public BaseGestureTest
{
public:
    void SetUp() override
    {
        BaseGestureTest::SetUp();

        // configure
        singleTapOneSecond.maxSecondsHeld = 1.0f;
        singleTapOneSecond.minClicksOrTaps = 1;
    }

    void TearDown() override
    {
        BaseGestureTest::TearDown();
    }
};


TEST_F(SimpleTests, NoInput_DefaultConfig_NotRecognized)
{
    MockRecognizer mockRecognizer;
    mockRecognizer.SetConfig(singleTapOneSecond);
    ASSERT_EQ(0, mockRecognizer.m_count);
}

TEST_F(SimpleTests, Tap_ZeroDuration_Recognized)
{
    MockRecognizer mockRecognizer;
    mockRecognizer.SetConfig(singleTapOneSecond);

    MouseDownAt(mockRecognizer, 0.0f);
    MouseUpAt(mockRecognizer, 0.0f);

    ASSERT_EQ(1, mockRecognizer.m_count);
}

TEST_F(SimpleTests, Tap_LessThanMaxDuration_Recognized)
{
    MockRecognizer mockRecognizer;
    mockRecognizer.SetConfig(singleTapOneSecond);

    MouseDownAt(mockRecognizer, 0.0f);
    MouseUpAt(mockRecognizer, 0.9f);

    ASSERT_EQ(1, mockRecognizer.m_count);
}

TEST_F(SimpleTests, Tap_GreaterThanMaxDuration_NotRecognized)
{
    MockRecognizer mockRecognizer;
    mockRecognizer.SetConfig(singleTapOneSecond);

    MouseDownAt(mockRecognizer, 0.0f);
    MouseUpAt(mockRecognizer, 1.1f);

    ASSERT_EQ(0, mockRecognizer.m_count);
}

TEST_F(SimpleTests, Tap_MoveWithinLimits_Recognized)
{
    MockRecognizer mockRecognizer;
    singleTapOneSecond.maxPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(singleTapOneSecond);

    MoveTo(0.0f, 0.0f);
    MouseDownAt(mockRecognizer, 0.0f);
    MoveTo(9.9f, 0.0f);
    MouseUpAt(mockRecognizer, 0.5f);

    ASSERT_EQ(1, mockRecognizer.m_count);
}

TEST_F(SimpleTests, Tap_MoveOutsideLimits_NotRecognized)
{
    MockRecognizer mockRecognizer;
    singleTapOneSecond.maxPixelsMoved = 10.0f;
    mockRecognizer.SetConfig(singleTapOneSecond);

    MoveTo(0.0f, 0.0f);
    MouseDownAt(mockRecognizer, 0.0f);
    MoveTo(10.1f, 0.0f);
    MouseUpAt(mockRecognizer, 0.5f);

    ASSERT_EQ(0, mockRecognizer.m_count);
}
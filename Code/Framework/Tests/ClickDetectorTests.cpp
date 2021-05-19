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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    std::ostream& operator<<(std::ostream& os, const ClickDetector::ClickOutcome clickOutcome)
    {
        switch (clickOutcome)
        {
        case ClickDetector::ClickOutcome::Click:
            os << "ClickOutcome::Click";
            break;
        case ClickDetector::ClickOutcome::Move:
            os << "ClickOutcome::Move";
            break;
        case ClickDetector::ClickOutcome::Release:
            os << "ClickOutcome::Release";
            break;
        case ClickDetector::ClickOutcome::Nil:
            os << "ClickOutcome::Nil";
            break;
        }

        return os;
    }
} // namespace AzFramework

namespace UnitTest
{
    using AzFramework::ClickDetector;
    using AzFramework::ScreenVector;

    class ClickDetectorFixture : public ::testing::Test
    {
    public:
        ClickDetector m_clickDetector;
    };

    TEST_F(ClickDetectorFixture, ClickIsDetectedWithNoMouseMovementOnMouseUp)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome initialUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
    }

    TEST_F(ClickDetectorFixture, MoveIsDetectedWithMouseMovementAfterMouseDown)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome initialMoveOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Nil, ScreenVector(10, 10));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialMoveOutcome, Eq(ClickDetector::ClickOutcome::Move));
    }

    TEST_F(ClickDetectorFixture, ReleaseIsDetectedAfterMouseMovementOnMouseUp)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        // move
        m_clickDetector.DetectClick(ClickDetector::ClickEvent::Nil, ScreenVector(10, 10));
        const ClickDetector::ClickOutcome initialUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialUpOutcome, Eq(ClickDetector::ClickOutcome::Release));
    }

    TEST_F(ClickDetectorFixture, MoveIsReturnedOnlyAfterFirstMouseMove)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome initialMoveOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Nil, ScreenVector(10, 10));
        const ClickDetector::ClickOutcome secondaryMoveOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Nil, ScreenVector(10, 10));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialMoveOutcome, Eq(ClickDetector::ClickOutcome::Move));
        EXPECT_THAT(secondaryMoveOutcome, Eq(ClickDetector::ClickOutcome::Nil));
    }

    TEST_F(ClickDetectorFixture, ClickIsNotRegisteredAfterDoubleClick)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome initialUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondaryDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondaryUpOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
        EXPECT_THAT(secondaryDownOutcome, Eq(ClickDetector::ClickOutcome::Nil)); // double click
        EXPECT_THAT(secondaryUpOutcome, Eq(ClickDetector::ClickOutcome::Nil)); // click not registered
    }

    TEST_F(ClickDetectorFixture, ClickIsNotRegisteredAfterIgnoredDoubleClick)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome initialDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome initialUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondaryDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Nil, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondaryUpOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(initialDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(initialUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
        EXPECT_THAT(secondaryDownOutcome, Eq(ClickDetector::ClickOutcome::Nil)); // ignored double click
        EXPECT_THAT(secondaryUpOutcome, Eq(ClickDetector::ClickOutcome::Nil)); // click not registered
    }
} // namespace UnitTest

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    // if the click detector registers a mouse down event, but then all intermediate calls are ignored
    // (another system may start intercepting events and swallowing them) then when we do receive a mouse
    // up event we should ensure we take into account the current delta - if the delta is large, then the
    // outcome will be release
    TEST_F(ClickDetectorFixture, ClickIsNotRegisteredAfterIgnoringMouseMovesBeforeMouseUpWithLargeDelta)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome downOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome upOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(50, 50));

        EXPECT_THAT(downOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(upOutcome, Eq(ClickDetector::ClickOutcome::Release));
    }

    //! note: ClickDetector does not explicitly return double clicks but if one occurs the ClickOutcome will be Nil
    TEST_F(ClickDetectorFixture, DoubleClickIsRegisteredIfMouseDeltaHasMovedLessThanDeadzoneInClickInterval)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome firstDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome firstUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(firstDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(firstUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
        EXPECT_THAT(secondDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(secondUpOutcome, Eq(ClickDetector::ClickOutcome::Nil));
    }

    TEST_F(ClickDetectorFixture, DoubleClickIsNotRegisteredIfMouseDeltaHasMovedMoreThanDeadzoneInClickInterval)
    {
        using ::testing::Eq;

        const ClickDetector::ClickOutcome firstDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome firstUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));
        const ClickDetector::ClickOutcome secondDownOutcome =
            m_clickDetector.DetectClick(ClickDetector::ClickEvent::Down, ScreenVector(10, 10));
        const ClickDetector::ClickOutcome secondUpOutcome = m_clickDetector.DetectClick(ClickDetector::ClickEvent::Up, ScreenVector(0, 0));

        EXPECT_THAT(firstDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(firstUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
        EXPECT_THAT(secondDownOutcome, Eq(ClickDetector::ClickOutcome::Nil));
        EXPECT_THAT(secondUpOutcome, Eq(ClickDetector::ClickOutcome::Click));
    }
} // namespace UnitTest

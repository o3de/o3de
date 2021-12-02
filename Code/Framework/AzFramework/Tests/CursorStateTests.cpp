/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CursorState.h>
#include <Tests/Utils/Printers.h>

namespace UnitTest
{
    using AzFramework::CursorState;
    using AzFramework::ScreenPoint;
    using AzFramework::ScreenVector;

    class CursorStateFixture : public ::testing::Test
    {
    public:
        CursorState m_cursorState;
    };

    TEST_F(CursorStateFixture, CursorStateHasZeroDeltaInitially)
    {
        using ::testing::Eq;
        EXPECT_THAT(m_cursorState.CursorDelta(), Eq(ScreenVector(0, 0)));
    }

    TEST_F(CursorStateFixture, CursorStateReturnsZeroDeltaAfterSingleMoveAndUpdate)
    {
        using ::testing::Eq;

        m_cursorState.SetCurrentPosition(ScreenPoint(10, 10));
        m_cursorState.Update();

        EXPECT_THAT(m_cursorState.CursorDelta(), Eq(ScreenVector(0, 0)));
    }

    TEST_F(CursorStateFixture, CursorStateReturnsDeltaAfterSecondMoveAndUpdate)
    {
        using ::testing::Eq;

        m_cursorState.SetCurrentPosition(ScreenPoint(10, 10));
        m_cursorState.Update();
        m_cursorState.SetCurrentPosition(ScreenPoint(15, 22));

        EXPECT_THAT(m_cursorState.CursorDelta(), Eq(ScreenVector(5, 12)));
    }
} // namespace UnitTest

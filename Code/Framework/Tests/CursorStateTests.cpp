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
#include <AzFramework/Viewport/CursorState.h>

namespace UnitTest
{
    using AzFramework::CursorState;
    using AzFramework::ScreenVector;
    using AzFramework::ScreenPoint;

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

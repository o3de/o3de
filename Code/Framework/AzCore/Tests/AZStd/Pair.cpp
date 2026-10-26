/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    class PairTestFixture
        : public LeakDetectionFixture
    {};

    TEST_F(PairTestFixture, StructuredBinding_ToConstAutoVar_CompilesSuccessfully)
    {
        constexpr AZStd::pair<int, int> testPair{ 3, 4 };
        const auto [firstElement, secondElement] = testPair;
        static_assert(AZStd::is_same_v<const int, decltype(firstElement)>);
        static_assert(AZStd::is_same_v<const int, decltype(secondElement)>);
        EXPECT_EQ(3, firstElement);
        EXPECT_EQ(4, secondElement);
    }

    TEST_F(PairTestFixture, CanGetFromAPairRValue)
    {
        {
            int myValue = 42;
            AZStd::pair<int&, int> myPair(myValue, 0);
            int& valueRef = AZStd::get<0>(AZStd::move(myPair));
            EXPECT_EQ(&myValue, &valueRef);

            static_assert(AZStd::is_same_v<decltype(AZStd::get<0>(AZStd::move(myPair))), int&>,
                    "Invoking AZStd::get on an lvalue reference element in a pair should return an lvalue reference");
        }

        {
            struct MoveOnlyType
            {
                MoveOnlyType() = default;
                MoveOnlyType(const MoveOnlyType&) = delete;
                MoveOnlyType(MoveOnlyType&&) = default;
            };
            MoveOnlyType myValue;
            const AZStd::pair<MoveOnlyType&&, int> myPair(AZStd::move(myValue), 42);

            MoveOnlyType&& valueRef = AZStd::get<0>(AZStd::move(myPair));
            EXPECT_EQ(&myValue, &valueRef);
            static_assert(AZStd::is_same_v<decltype(AZStd::get<0>(AZStd::move(myPair))), MoveOnlyType&&>,
                    "Invoking AZStd::get on an rvalue reference element in a pair should return an rvalue reference");
        }
    }
}

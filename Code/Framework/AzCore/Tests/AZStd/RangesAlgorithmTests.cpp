/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace UnitTest
{
    class RangesAlgorithmTestFixture
        : public ScopedAllocatorSetupFixture
    {};

    // range algorithm min and max
    TEST_F(RangesAlgorithmTestFixture, RangesMin_ReturnsSmallestElement)
    {
        // Simple Elements
        EXPECT_EQ(-1, AZStd::ranges::min(5, -1));
        EXPECT_EQ(78235, AZStd::ranges::min(78235, 124785));
        EXPECT_EQ(7, AZStd::ranges::min(7, 7));

        // Initializer list
        AZStd::initializer_list testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        EXPECT_EQ(-8, AZStd::ranges::min(testIList));

        // Range
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        EXPECT_EQ(-8, AZStd::ranges::min(testVector));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMax_ReturnsLargestElement)
    {
        // Simple Elements
        EXPECT_EQ(5, AZStd::ranges::max(5, -1));
        EXPECT_EQ(124785, AZStd::ranges::max(78235, 124785));
        EXPECT_EQ(7, AZStd::ranges::max(7, 7));

        // Initializer list
        AZStd::initializer_list testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        EXPECT_EQ(1000, AZStd::ranges::max(testIList));

        // Range
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        EXPECT_EQ(1000, AZStd::ranges::max(testVector));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMinMax_ReturnsSmallestAndLargestValue)
    {
        // Simple Elements
        AZStd::ranges::minmax_result<int> expectedMinMax{ -1, 5 };
        AZStd::ranges::minmax_result<int> testMinMax = AZStd::ranges::minmax(5, -1);
        EXPECT_EQ(expectedMinMax.min, testMinMax.min);
        EXPECT_EQ(expectedMinMax.max, testMinMax.max);

        expectedMinMax = { 78235, 124785 };
        testMinMax = AZStd::ranges::minmax(78235, 124785);
        EXPECT_EQ(expectedMinMax.min, testMinMax.min);
        EXPECT_EQ(expectedMinMax.max, testMinMax.max);

        expectedMinMax = { 7, 7 };
        testMinMax = AZStd::ranges::minmax(7, 7);
        EXPECT_EQ(expectedMinMax.min, testMinMax.min);
        EXPECT_EQ(expectedMinMax.max, testMinMax.max);

        // Initializer list
        AZStd::initializer_list testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        expectedMinMax = { -8, 1000 };
        testMinMax = AZStd::ranges::minmax(testIList);
        EXPECT_EQ(expectedMinMax.min, testMinMax.min);
        EXPECT_EQ(expectedMinMax.max, testMinMax.max);

        // Range
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        expectedMinMax = { -8, 1000 };
        testMinMax = AZStd::ranges::minmax(testVector);
        EXPECT_EQ(expectedMinMax.min, testMinMax.min);
        EXPECT_EQ(expectedMinMax.max, testMinMax.max);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMinElement_ReturnsIteratorToLeftmostSmallestElement)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        // iterator
        auto leftmostSmallestIt = AZStd::ranges::min_element(testVector.begin(), testVector.end());
        ASSERT_EQ(testVector.begin() + 4, leftmostSmallestIt);
        EXPECT_EQ(-8, *leftmostSmallestIt);

        // Range
        leftmostSmallestIt = AZStd::ranges::min_element(testVector);
        ASSERT_EQ(testVector.begin() + 4, leftmostSmallestIt);
        EXPECT_EQ(-8, *leftmostSmallestIt);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMaxElement_ReturnsIteratorToLeftmostLargestElement)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        // iterator
        auto leftmostLargestIt = AZStd::ranges::max_element(testVector.begin(), testVector.end());
        ASSERT_EQ(testVector.begin() + 6, leftmostLargestIt);
        EXPECT_EQ(1000, *leftmostLargestIt);

        // Range
        leftmostLargestIt = AZStd::ranges::max_element(testVector);
        ASSERT_EQ(testVector.begin() + 6, leftmostLargestIt);
        EXPECT_EQ(1000, *leftmostLargestIt);
    }
    TEST_F(RangesAlgorithmTestFixture, RangesMinMaxElement_ReturnsIteratorToLeftmostSmallestElement_IteratorToRightmostLargestElement)
    {
        // The behavior of minmax_element is explicitly different from max_element
        // as it relates to the max element being returned
        // mixmax_element returns the rightmost largest element(assuming comparison function object is ranges::less)
        // https://eel.is/c++draft/algorithms#footnoteref-225
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
        // iterator
        {
            auto [leftmostSmallestIt, rightmostLargestIt] = AZStd::ranges::minmax_element(testVector.begin(), testVector.end());
            ASSERT_EQ(testVector.begin() + 4, leftmostSmallestIt);
            ASSERT_EQ(testVector.begin() + 10, rightmostLargestIt);
            EXPECT_EQ(-8, *leftmostSmallestIt);
            EXPECT_EQ(1000, *rightmostLargestIt);
        }

        // Range
        auto [leftmostSmallestIt, rightmostLargestIt] = AZStd::ranges::minmax_element(testVector);
        ASSERT_EQ(testVector.begin() + 4, leftmostSmallestIt);
        ASSERT_EQ(testVector.begin() + 10, rightmostLargestIt);
        EXPECT_EQ(-8, *leftmostSmallestIt);
        EXPECT_EQ(1000, *rightmostLargestIt);
    }
}

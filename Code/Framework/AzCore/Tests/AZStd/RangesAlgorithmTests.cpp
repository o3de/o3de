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
        AZStd::initializer_list<int> testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
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
        AZStd::initializer_list<int> testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
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
        AZStd::initializer_list<int> testIList{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };
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

    TEST_F(RangesAlgorithmTestFixture, RangesFind_LocatesElementInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };

        auto foundIt = AZStd::ranges::find(testVector, 22);
        ASSERT_NE(testVector.end(), foundIt);
        EXPECT_EQ(22, *foundIt);

        foundIt = AZStd::ranges::find_if(testVector, [](int value) { return value < 0; });
        ASSERT_NE(testVector.end(), foundIt);
        EXPECT_EQ(-8, *foundIt);

        foundIt = AZStd::ranges::find_if_not(testVector, [](int value) { return value < 1000; });
        ASSERT_NE(testVector.end(), foundIt);
        EXPECT_EQ(1000, *foundIt);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesFindFirstOf_LocatesElementInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };

        AZStd::array testArray{ -8, 47 };
        auto foundIt = AZStd::ranges::find_first_of(testVector, testArray);
        ASSERT_NE(testVector.end(), foundIt);
        EXPECT_EQ(47, *foundIt);

        AZStd::array<int, 0> emptyArray{};
        foundIt = AZStd::ranges::find_first_of(testVector, emptyArray);
        EXPECT_EQ(testVector.end(), foundIt);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesSearch_LocatesElementInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45 };

        AZStd::array testArray{ 1000 };
        auto testSubrange = AZStd::ranges::search(testVector, testArray);
        ASSERT_FALSE(testSubrange.empty());
        EXPECT_EQ(1000, testSubrange.front());

        AZStd::array testArray2{ 1000, 45 };
        testSubrange = AZStd::ranges::search(testVector, testArray2);
        ASSERT_EQ(2, testSubrange.size());
        EXPECT_EQ(1000, testSubrange[0]);
        EXPECT_EQ(45, testSubrange[1]);

        testSubrange = AZStd::ranges::search_n(testVector, 1, 22);
        ASSERT_FALSE(testSubrange.empty());
        EXPECT_EQ(22, testSubrange.front());

        // 2 Consecutive values of 22 does not exist in vector
        testSubrange = AZStd::ranges::search_n(testVector, 2, 22);
        EXPECT_TRUE(testSubrange.empty());

        // 2 Consecutive values of -8 does exist in vector
        testSubrange = AZStd::ranges::search_n(testVector, 2,- 8);
        ASSERT_EQ(2, testSubrange.size());
        EXPECT_EQ(-8, testSubrange[0]);
        EXPECT_EQ(-8, testSubrange[1]);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesFindEnd_LocatesElementLastElement_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45 };

        AZStd::array testArray{ -8 };
        auto testSubrange = AZStd::ranges::find_end(testVector, testArray);
        ASSERT_FALSE(testSubrange.empty());
        EXPECT_EQ(-8, testSubrange.front());
        EXPECT_EQ(testVector.end() - 3, testSubrange.begin());
    }

    TEST_F(RangesAlgorithmTestFixture, RangesEqual_IsAbleToCompareTwoRanges_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45 };
        AZStd::vector longerVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45, 929 };
        AZStd::vector shorterVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000 };
        AZStd::vector unequalVector{ 5, 1, 22, 47, -8, -5, 1000, 14, 22, -8, -8, 1000, 25 };

        EXPECT_TRUE(AZStd::ranges::equal(testVector, testVector));
        EXPECT_FALSE(AZStd::ranges::equal(testVector, longerVector));
        EXPECT_FALSE(AZStd::ranges::equal(longerVector, testVector));
        EXPECT_FALSE(AZStd::ranges::equal(testVector, shorterVector));
        EXPECT_FALSE(AZStd::ranges::equal(shorterVector, testVector));
        EXPECT_FALSE(AZStd::ranges::equal(testVector, unequalVector));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMismatch_Returns_IteratorsToMismatchElements)
    {
        AZStd::vector testVector{ 1, 2, 3, 4, 5 ,6 };
        AZStd::vector secondVector{ 1, 2, 3, 14, 5, 6 };
        AZStd::vector<int> emptyVector;
        AZStd::vector<int> longerVector{ 1, 2, 3, 4, 5, 6, 7 };

        {
            auto [mismatchIt1, mismatchIt2] = AZStd::ranges::mismatch(testVector, secondVector);
            ASSERT_NE(testVector.end(), mismatchIt1);
            EXPECT_EQ(4, *mismatchIt1);
            ASSERT_NE(secondVector.end(), mismatchIt2);
            EXPECT_EQ(14, *mismatchIt2);
        }
        {
            auto [mismatchIt1, mismatchIt2] = AZStd::ranges::mismatch(testVector, emptyVector);
            ASSERT_NE(testVector.end(), mismatchIt1);
            EXPECT_EQ(1, *mismatchIt1);
            EXPECT_EQ(emptyVector.end(), mismatchIt2);
        }
        {
            auto [mismatchIt1, mismatchIt2] = AZStd::ranges::mismatch(testVector, longerVector);
            EXPECT_EQ(testVector.end(), mismatchIt1);
            ASSERT_NE(longerVector.end(), mismatchIt2);
            EXPECT_EQ(7, *mismatchIt2);
        }
    }
}

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

    TEST_F(RangesAlgorithmTestFixture, RangesAllOfReturnsTrueForMatchingViews)
    {
        constexpr AZStd::array numbers {0, 1, 2, 3, 4, 5};
        EXPECT_TRUE(AZStd::ranges::all_of(numbers, [](int i) { return i < 6; })) << "All numbers should be less than 6";
        EXPECT_FALSE(AZStd::ranges::all_of(numbers, [](int i) { return i < 5; })) << "At least one number should be greater than or equal to 5";
    }

    TEST_F(RangesAlgorithmTestFixture, RangesAnyOfReturnsTrueForMatchingViews)
    {
        constexpr AZStd::array numbers {0, 1, 2, 3, 4, 5};
        EXPECT_TRUE(AZStd::ranges::any_of(numbers, [](int i) { return i == 3; })) << "At least one number should equal 3";
        EXPECT_FALSE(AZStd::ranges::any_of(numbers, [](int i) { return i == 6; })) << "No number should equal 6";
    }

    TEST_F(RangesAlgorithmTestFixture, RangesForEach_LoopsOverElements_Success)
    {
        constexpr AZStd::string_view expectedString = "HelloWorldLongString";
        constexpr AZStd::array words{ "Hello", "World", "Long", "String" };

        {
            // Check for_each which accepts iterators
            AZStd::string resultString;
            AZStd::ranges::for_each(words.begin(), words.end(), [&resultString](AZStd::string_view elem) { resultString += elem; });
            EXPECT_EQ(expectedString, resultString);
        }
        {
            // Check for_each which accepts a range
            AZStd::string resultString;
            AZStd::ranges::for_each(words, [&resultString](AZStd::string_view elem) { resultString += elem; });
            EXPECT_EQ(expectedString, resultString);
        }
        {
            // Check for_each_n which accepts an iterator and a count
            constexpr AZStd::string_view expectedForEachString = "HelloWorld";
            AZStd::string resultString;
            AZStd::ranges::for_each_n(words.begin(), 2, [&resultString](AZStd::string_view elem) { resultString += elem; });
            EXPECT_EQ(expectedForEachString, resultString);
        }
    }
    TEST_F(RangesAlgorithmTestFixture, RangesCount_CountsCharacters_Succeeds)
    {
        constexpr AZStd::string_view sourceString = "HelloWorldLongString";
        constexpr size_t expectedChar_o_count = 3;

        {
            // Check count which accepts iterators
            EXPECT_EQ(expectedChar_o_count, AZStd::ranges::count(sourceString.begin(), sourceString.end(), 'o'));
        }
        {
            // Check count which accepts a range
            EXPECT_EQ(expectedChar_o_count, AZStd::ranges::count(sourceString, 'o'));
        }
        {
            // Check count_if which accepts iterators and a unary predicate
            constexpr size_t expectedChar_l_count = 3;
            auto CountLetter_l = [](char elem) { return elem == 'l'; };
            EXPECT_EQ(expectedChar_l_count, AZStd::ranges::count_if(sourceString.begin(), sourceString.end(), AZStd::move(CountLetter_l)));
        }
        {
            // Check count_if which accepts a range and a unary predicate
            constexpr size_t expectedChar_r_count = 2;
            auto CountLetter_r = [](char elem) { return elem == 'r'; };
            EXPECT_EQ(expectedChar_r_count, AZStd::ranges::count_if(sourceString, AZStd::move(CountLetter_r)));
        }
    }

    TEST_F(RangesAlgorithmTestFixture, RangesCopy_CopyRangeIntoOutputIterator_Succeeds)
    {
        constexpr AZStd::string_view sourceString = "HelloWorld";

        AZStd::vector<char> charVector;
        AZStd::ranges::copy(sourceString, AZStd::back_inserter(charVector));
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        charVector.clear();

        AZStd::ranges::copy(sourceString.begin(), sourceString.end(), AZStd::back_inserter(charVector));
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));

        auto SkipCapitalLetters = [](const char elem) -> bool
        {
            return islower(elem);
        };
        charVector.clear();
        AZStd::ranges::copy_if(sourceString, AZStd::back_inserter(charVector), SkipCapitalLetters);
        EXPECT_THAT(charVector, ::testing::ElementsAre('e', 'l', 'l', 'o', 'o', 'r', 'l', 'd'));
        charVector.clear();
        AZStd::ranges::copy_if(sourceString.begin(), sourceString.end(), AZStd::back_inserter(charVector), SkipCapitalLetters);
        EXPECT_THAT(charVector, ::testing::ElementsAre('e', 'l', 'l', 'o', 'o', 'r', 'l', 'd'));

        charVector.clear();
        AZStd::ranges::copy_n(sourceString.begin(), 5, AZStd::back_inserter(charVector));
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o'));

        charVector.clear();
        charVector.resize_no_construct(sourceString.size());
        AZStd::ranges::copy_backward(sourceString, charVector.end());
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        charVector.clear();
        charVector.resize_no_construct(sourceString.size());
        AZStd::ranges::copy_backward(sourceString.begin(), sourceString.end(), charVector.end());
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesMove_MoveElementsIntoOutputIterator_Succeeds)
    {
        struct MoveOpRemoveContents
        {
            MoveOpRemoveContents() = default;
            MoveOpRemoveContents(const MoveOpRemoveContents&) = default;
            MoveOpRemoveContents& operator=(const MoveOpRemoveContents&) = default;

            MoveOpRemoveContents(char elem)
                : m_elem{elem}
            {}
            MoveOpRemoveContents(MoveOpRemoveContents&& other)
                : m_elem{ other.m_elem }
            {
                // Reset the other element the NUL character
                other.m_elem = {};
            }
            MoveOpRemoveContents& operator=(MoveOpRemoveContents&& other)
            {
                m_elem = other.m_elem;
                other.m_elem = {};
                return *this;
            }

            char m_elem{};

            bool operator==(const MoveOpRemoveContents& other) const
            {
                return m_elem == other.m_elem;
            }
            bool operator!=(const MoveOpRemoveContents& other) const
            {
                return !operator==(other);
            }
        };

        // testString will be moved several times in this test
        // So it the contents need to be re-initialized after each ranges::move* call
        AZStd::vector<MoveOpRemoveContents> testString{ 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd' };

        AZStd::vector<MoveOpRemoveContents> charVector;
        AZStd::ranges::move(testString, AZStd::back_inserter(charVector));
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        EXPECT_THAT(testString, ::testing::ElementsAre('\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'));

        charVector.clear();
        testString = AZStd::vector<MoveOpRemoveContents>{ 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd' };
        AZStd::ranges::move(testString.begin(), testString.end(), AZStd::back_inserter(charVector));
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        EXPECT_THAT(testString, ::testing::ElementsAre('\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'));

        charVector.clear();
        testString = AZStd::vector<MoveOpRemoveContents>{ 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd' };
        charVector.resize_no_construct(testString.size());
        AZStd::ranges::move_backward(testString, charVector.end());
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        EXPECT_THAT(testString, ::testing::ElementsAre('\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'));

        charVector.clear();
        testString = AZStd::vector<MoveOpRemoveContents>{ 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd' };
        charVector.resize_no_construct(testString.size());
        AZStd::ranges::move_backward(testString.begin(), testString.end(), charVector.end());
        EXPECT_THAT(charVector, ::testing::ElementsAre('H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'));
        EXPECT_THAT(testString, ::testing::ElementsAre('\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'));
    }
}

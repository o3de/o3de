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
#include <AzCore/std/ranges/ranges_functional.h>

namespace UnitTest
{
    class RangesAlgorithmTestFixture
        : public LeakDetectionFixture
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

    TEST_F(RangesAlgorithmTestFixture, RangesFindLast_LocatesLastMatchInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };

        auto foundSubrange = AZStd::ranges::find_last(testVector, 22);
        ASSERT_NE(testVector.begin() + 7, foundSubrange.begin());
        EXPECT_EQ(22, *foundSubrange.begin());

        foundSubrange = AZStd::ranges::find_last_if(testVector, [](int value) { return value < 0; });
        ASSERT_NE(testVector.begin() + 8, foundSubrange.begin());
        EXPECT_EQ(-8, *foundSubrange.begin());

        foundSubrange = AZStd::ranges::find_last_if_not(testVector, [](int value) { return value < 1000; });
        ASSERT_NE(testVector.begin() + 9, foundSubrange.begin());
        EXPECT_EQ(1000, *foundSubrange.begin());
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

    TEST_F(RangesAlgorithmTestFixture, RangesLexicographicalCompare_IsAbleToCompareTwoRanges_ReturnsSmallerRange)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45 };
        AZStd::vector longerVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45, 929 };
        AZStd::vector shorterVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000 };
        AZStd::vector unequalVector{ 5, 1, 22, 47, -8, -5, 1000, 14, 22, -8, -8, 1000, 25 };

        const AZStd::vector<int> emptyVector;
        EXPECT_FALSE(AZStd::ranges::lexicographical_compare(emptyVector, emptyVector));
        EXPECT_TRUE(AZStd::ranges::lexicographical_compare(emptyVector, testVector));
        EXPECT_FALSE(AZStd::ranges::lexicographical_compare(testVector, emptyVector));

        // testVector is a proper prefix of longerVector
        EXPECT_TRUE(AZStd::ranges::lexicographical_compare(testVector, longerVector));
        EXPECT_FALSE(AZStd::ranges::lexicographical_compare(longerVector, testVector));

        // shorterVector is a proper prefix of shorterVector
        EXPECT_FALSE(AZStd::ranges::lexicographical_compare(testVector, shorterVector));
        EXPECT_TRUE(AZStd::ranges::lexicographical_compare(shorterVector, testVector));

        // testVector and unequalVector are the same size, but have different element values
        EXPECT_FALSE(AZStd::ranges::lexicographical_compare(testVector, unequalVector));
        EXPECT_TRUE(AZStd::ranges::lexicographical_compare(unequalVector, testVector));
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

    TEST_F(RangesAlgorithmTestFixture, RangesAllOf_ReturnsTrueForMatchingViews)
    {
        constexpr AZStd::array numbers {0, 1, 2, 3, 4, 5};
        EXPECT_TRUE(AZStd::ranges::all_of(numbers, [](int i) { return i < 6; })) << "All numbers should be less than 6";
        EXPECT_FALSE(AZStd::ranges::all_of(numbers, [](int i) { return i < 5; })) << "At least one number should be greater than or equal to 5";
    }

    TEST_F(RangesAlgorithmTestFixture, RangesAnyOf_ReturnsTrueForMatchingViews)
    {
        constexpr AZStd::array numbers {0, 1, 2, 3, 4, 5};
        EXPECT_TRUE(AZStd::ranges::any_of(numbers, [](int i) { return i == 3; })) << "At least one number should equal 3";
        EXPECT_FALSE(AZStd::ranges::any_of(numbers, [](int i) { return i == 6; })) << "No number should equal 6";
    }

    TEST_F(RangesAlgorithmTestFixture, RangesNoneOf_ReturnsTrueForNoMatchingViews)
    {
        constexpr AZStd::array numbers{ 0, 1, 2, 3, 4, 5 };
        EXPECT_TRUE(AZStd::ranges::none_of(numbers, [](int i) { return i == 10; })) << "No number should equal 10";
        EXPECT_FALSE(AZStd::ranges::none_of(numbers, [](int i) { return i == 4; })) << "At least one number should equal 4";
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

    TEST_F(RangesAlgorithmTestFixture, RangesTransform_UnaryConvertsInPlace_Succeeds)
    {
        const char* expectedString = "ifmmp";
        AZStd::string testString = "hello";
        constexpr auto incrementChar = [](char elem) { return static_cast<char>(elem + 1); };
        auto unaryResult = AZStd::ranges::transform(testString, testString.begin(), incrementChar);
        EXPECT_EQ(testString.end(), unaryResult.in);
        EXPECT_EQ(testString.end(), unaryResult.out);
        EXPECT_EQ(expectedString, testString);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesTransform_UnaryModifiesOtherRange_Succeeds)
    {
        AZStd::string testString = "hello";
        AZStd::vector<int> ordinals;
        auto unaryResult = AZStd::ranges::transform(testString, AZStd::back_inserter(ordinals), AZStd::identity{});
        EXPECT_EQ(testString.end(), unaryResult.in);

        EXPECT_THAT(ordinals, ::testing::ElementsAre('h', 'e', 'l', 'l', 'o'));
    }

     TEST_F(RangesAlgorithmTestFixture, RangesTransform_BinaryCombinesTwoRanges_Succeeds)
    {
        AZStd::array testArray1{1, 4, 9, 16};
        AZStd::array testArray2{2, 4, 6, 8};
        AZStd::vector<int> ordinals;
        constexpr auto AddNumbers = [](int left, int right) { return left + right; };
        auto binaryResult = AZStd::ranges::transform(testArray1, testArray2, AZStd::back_inserter(ordinals), AddNumbers);
        EXPECT_EQ(testArray1.end(), binaryResult.in1);
        EXPECT_EQ(testArray2.end(), binaryResult.in2);

        EXPECT_THAT(ordinals, ::testing::ElementsAre(3, 8, 15, 24));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesTransform_ModifiesOtherRange_Succeeds)
    {
        AZStd::string testString = "hello";
        AZStd::vector<int> ordinals;
        auto unaryResult = AZStd::ranges::transform(testString, AZStd::back_inserter(ordinals), AZStd::identity{});
        EXPECT_EQ(testString.end(), unaryResult.in);

        EXPECT_THAT(ordinals, ::testing::ElementsAre('h', 'e', 'l', 'l', 'o'));
    }


    struct ReverseMoveOnly
    {
        ReverseMoveOnly() = default;
        ReverseMoveOnly(const ReverseMoveOnly&) = delete;
        ReverseMoveOnly& operator=(const ReverseMoveOnly&) = delete;
        ReverseMoveOnly(ReverseMoveOnly&&) = default;
        ReverseMoveOnly& operator=(ReverseMoveOnly&&) = default;

        friend bool operator==(const ReverseMoveOnly& left, const ReverseMoveOnly& right)
        {
            return left.m_value == right.m_value;
        }

        friend bool operator!=(const ReverseMoveOnly& left, const ReverseMoveOnly& right)
        {
            return !operator==(left, right);
        }

        int m_value{};
    };
    TEST_F(RangesAlgorithmTestFixture, RangesReverse_SwapsInplace_Succeeds)
    {
        AZStd::array testArray{ ReverseMoveOnly{ 0 }, ReverseMoveOnly{ 1 }, ReverseMoveOnly{ 2 }, ReverseMoveOnly{ 3 } };

        auto endOfRangeIter = AZStd::ranges::reverse(testArray);
        EXPECT_EQ(testArray.end(), endOfRangeIter);

        constexpr AZStd::array expectedArray{ ReverseMoveOnly{ 3 }, ReverseMoveOnly{ 2 }, ReverseMoveOnly{ 1 }, ReverseMoveOnly{ 0 } };

        EXPECT_EQ(testArray, expectedArray);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesContains_LocatesElementInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };

        EXPECT_TRUE(AZStd::ranges::contains(testVector, 22));
        EXPECT_TRUE(AZStd::ranges::contains(testVector.begin(), testVector.end(), 1000));
        EXPECT_FALSE(AZStd::ranges::contains(testVector, -19));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesContains_LocatesSubrangeInContainer_Succeeds)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, 1000, 45 };

        constexpr AZStd::array searchRange{ 687, 22 };
        EXPECT_TRUE(AZStd::ranges::contains_subrange(testVector, searchRange));
        EXPECT_TRUE(AZStd::ranges::contains_subrange(testVector.begin(), testVector.end(),
            searchRange.begin(), searchRange.end()));
        EXPECT_FALSE(AZStd::ranges::contains_subrange(testVector, AZStd::array{735, 32}));
        constexpr AZStd::array<int, 0> emptyRange{};
        EXPECT_TRUE(AZStd::ranges::contains_subrange(testVector, emptyRange));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesStartsWith_FindsSubrangeAtStart)
    {
        AZStd::vector testVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45 };
        AZStd::vector longerVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000, 45, 929 };
        AZStd::vector shorterVector{ 5, 1, 22, 47, -8, -5, 1000, 687, 22, -8, -8, 1000 };
        AZStd::vector unequalVector{ 5, 1, 22, 47, -8, -5, 1000, 14, 22, -8, -8, 1000, 25 };
        AZStd::vector<int> emptyVector;

        EXPECT_TRUE(AZStd::ranges::starts_with(testVector, emptyVector));
        EXPECT_TRUE(AZStd::ranges::starts_with(emptyVector, emptyVector));

        EXPECT_TRUE(AZStd::ranges::starts_with(testVector, testVector));
        EXPECT_FALSE(AZStd::ranges::starts_with(testVector, longerVector));
        EXPECT_TRUE(AZStd::ranges::starts_with(longerVector, testVector));

        EXPECT_TRUE(AZStd::ranges::starts_with(testVector, shorterVector));
        EXPECT_FALSE(AZStd::ranges::starts_with(shorterVector, testVector));
        EXPECT_FALSE(AZStd::ranges::starts_with(testVector, unequalVector));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesEndsWith_FindsSubrangeAtEnd)
    {
        AZStd::vector testVector{ 4, 5, 6, 7, 8, 9 };
        AZStd::vector longerVector{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        AZStd::vector shorterVector{ 7, 8, 9 };
        AZStd::vector<int> emptyVector;

        EXPECT_TRUE(AZStd::ranges::ends_with(testVector, emptyVector));
        EXPECT_TRUE(AZStd::ranges::ends_with(emptyVector, emptyVector));

        EXPECT_TRUE(AZStd::ranges::ends_with(testVector, testVector));
        EXPECT_FALSE(AZStd::ranges::ends_with(testVector, longerVector));
        EXPECT_TRUE(AZStd::ranges::ends_with(longerVector, testVector));

        EXPECT_TRUE(AZStd::ranges::ends_with(testVector, shorterVector));
        EXPECT_FALSE(AZStd::ranges::ends_with(shorterVector, testVector));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesLowerBound_CanProjectElementFromMoveOnlyContainer)
    {
        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly&&) = default;

            int m_value{};
        };
        AZStd::array testArray{ MoveOnly{0}, MoveOnly{1}, MoveOnly{2} , MoveOnly{3} , MoveOnly{5},
            MoveOnly{6}, MoveOnly{7} };

        int valueToSearch = 3;
        auto ProjectMoveOnlyToInt = [](const MoveOnly& moveOnly) { return moveOnly.m_value; };
        auto lowerBoundIter = AZStd::ranges::lower_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), lowerBoundIter);
        EXPECT_EQ(3, lowerBoundIter->m_value);

        valueToSearch = 4;
        lowerBoundIter = AZStd::ranges::lower_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), lowerBoundIter);
        // lower_bound is 5 since 4 is not in the array
        EXPECT_EQ(5, lowerBoundIter->m_value);

        valueToSearch = 0;
        lowerBoundIter = AZStd::ranges::lower_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), lowerBoundIter);
        EXPECT_EQ(0, lowerBoundIter->m_value);

        valueToSearch = 7;
        lowerBoundIter = AZStd::ranges::lower_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), lowerBoundIter);
        EXPECT_EQ(7, lowerBoundIter->m_value);

        valueToSearch = 8;
        lowerBoundIter = AZStd::ranges::lower_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        EXPECT_EQ(testArray.end(), lowerBoundIter);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesUpperBound_CanProjectElementFromMoveOnlyContainer)
    {
        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly&&) = default;

            int m_value{};
        };
        AZStd::array testArray{ MoveOnly{0}, MoveOnly{1}, MoveOnly{2} , MoveOnly{3} , MoveOnly{5},
            MoveOnly{6}, MoveOnly{7} };

        int valueToSearch = 3;
        auto ProjectMoveOnlyToInt = [](const MoveOnly& moveOnly) { return moveOnly.m_value; };
        auto upperBoundIter = AZStd::ranges::upper_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), upperBoundIter);
        // upper_bound is 5 as 4 is not an element in the array
        EXPECT_EQ(5, upperBoundIter->m_value);

        valueToSearch = 4;
        upperBoundIter = AZStd::ranges::upper_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), upperBoundIter);
        // upper_bound is still 5
        EXPECT_EQ(5, upperBoundIter->m_value);

        valueToSearch = 0;
        upperBoundIter = AZStd::ranges::upper_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_NE(testArray.end(), upperBoundIter);
        EXPECT_EQ(1, upperBoundIter->m_value);

        valueToSearch = 7;
        upperBoundIter = AZStd::ranges::upper_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        // There is no value above 7 in the array so the upper bound is the end
        EXPECT_EQ(testArray.end(), upperBoundIter);

        valueToSearch = 8;
        upperBoundIter = AZStd::ranges::upper_bound(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        EXPECT_EQ(testArray.end(), upperBoundIter);
    }

    TEST_F(RangesAlgorithmTestFixture, RangesEqualRange_CanProjectElementFromMoveOnlyContainer)
    {
        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly&&) = default;

            int m_value{};
        };
        AZStd::array testArray{ MoveOnly{0}, MoveOnly{1}, MoveOnly{2} , MoveOnly{3} , MoveOnly{5},
            MoveOnly{6}, MoveOnly{7} };

        int valueToSearch = 3;
        auto ProjectMoveOnlyToInt = [](const MoveOnly& moveOnly) { return moveOnly.m_value; };
        auto equalSubrange = AZStd::ranges::equal_range(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_EQ(1, equalSubrange.size());
        EXPECT_EQ(3, equalSubrange.front().m_value);

        valueToSearch = 4;
        equalSubrange = AZStd::ranges::equal_range(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        // 4 is not an element of the array
        EXPECT_TRUE(equalSubrange.empty());

        valueToSearch = 0;
        equalSubrange = AZStd::ranges::equal_range(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_EQ(1, equalSubrange.size());
        EXPECT_EQ(0, equalSubrange.front().m_value);

        valueToSearch = 7;
        equalSubrange = AZStd::ranges::equal_range(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        ASSERT_EQ(1, equalSubrange.size());
        EXPECT_EQ(7, equalSubrange.front().m_value);

        valueToSearch = 8;
        equalSubrange = AZStd::ranges::equal_range(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt);
        EXPECT_TRUE(equalSubrange.empty());
    }

    TEST_F(RangesAlgorithmTestFixture, RangesBinarySearch_CanProjectElementFromMoveOnlyContainer)
    {
        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly&&) = default;

            int m_value{};
        };
        AZStd::array testArray{ MoveOnly{0}, MoveOnly{1}, MoveOnly{2} , MoveOnly{3} , MoveOnly{5},
            MoveOnly{6}, MoveOnly{7} };

        int valueToSearch = 3;
        auto ProjectMoveOnlyToInt = [](const MoveOnly& moveOnly) { return moveOnly.m_value; };
        EXPECT_TRUE(AZStd::ranges::binary_search(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt));

        valueToSearch = 4;
        // 4 is not in the container
        EXPECT_FALSE(AZStd::ranges::binary_search(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt));

        valueToSearch = 0;
        EXPECT_TRUE(AZStd::ranges::binary_search(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt));

        valueToSearch = 7;
        EXPECT_TRUE(AZStd::ranges::binary_search(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt));

        valueToSearch = 8;
        EXPECT_FALSE(AZStd::ranges::binary_search(testArray, valueToSearch, AZStd::ranges::less{}, ProjectMoveOnlyToInt));
    }

    TEST_F(RangesAlgorithmTestFixture, RangesIota_IncrementsSequence_UntilOfRange)
    {
        constexpr int vectorSize = 10;
        AZStd::vector<int> testVector(vectorSize);
        constexpr int startValue = -4;
        auto result = AZStd::ranges::iota(testVector, startValue);

        EXPECT_EQ(startValue + vectorSize, result.value);
        EXPECT_EQ(testVector.end(), result.out);

        AZStd::vector<int> expectedVector;
        expectedVector.reserve(vectorSize);
        for (int i = 0; i < vectorSize; ++i)
        {
            expectedVector.push_back(startValue + i);
        }
        EXPECT_THAT(testVector, ::testing::ContainerEq(expectedVector));
    }
}

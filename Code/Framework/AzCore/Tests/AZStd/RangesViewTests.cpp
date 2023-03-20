/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/as_const_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/counted_view.h>
#include <AzCore/std/ranges/elements_view.h>
#include <AzCore/std/ranges/empty_view.h>
#include <AzCore/std/ranges/filter_view.h>
#include <AzCore/std/ranges/iota_view.h>
#include <AzCore/std/ranges/join_view.h>
#include <AzCore/std/ranges/join_with_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/ranges/repeat_view.h>
#include <AzCore/std/ranges/reverse_view.h>
#include <AzCore/std/ranges/single_view.h>
#include <AzCore/std/ranges/split_view.h>
#include <AzCore/std/ranges/subrange.h>
#include <AzCore/std/ranges/transform_view.h>
#include <AzCore/std/ranges/zip_view.h>
#include <AzCore/std/string/string_view.h>

namespace UnitTest
{
    class RangesViewTestFixture
        : public LeakDetectionFixture
    {};

    TEST_F(RangesViewTestFixture, AllRangeAdaptor_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto testAllView = AZStd::ranges::views::all(testString);
        EXPECT_EQ(testString.size(), testAllView.size());
        EXPECT_EQ(testString.data(), testAllView.data());
        EXPECT_EQ(testString.begin(), testAllView.begin());
        EXPECT_EQ(testString.end(), testAllView.end());
        EXPECT_EQ(testString.empty(), testAllView.empty());
        EXPECT_EQ(testString.front(), testAllView.front());
        EXPECT_EQ(testString.back(), testAllView.back());
        EXPECT_EQ(testString[5], testAllView[5]);

        auto testAllViewChain = testString | AZStd::ranges::views::all;
        EXPECT_EQ(testString.size(), testAllViewChain.size());
        EXPECT_EQ(testString.data(), testAllViewChain.data());
        EXPECT_EQ(testString.begin(), testAllViewChain.begin());
        EXPECT_EQ(testString.end(), testAllViewChain.end());
        EXPECT_EQ(testString.empty(), testAllViewChain.empty());
        EXPECT_EQ(testString.front(), testAllViewChain.front());
        EXPECT_EQ(testString.back(), testAllViewChain.back());
        EXPECT_EQ(testString[5], testAllViewChain[5]);
    }

    TEST_F(RangesViewTestFixture, Subrange_DeductionGuides_Compile)
    {
        AZStd::string_view testString{ "Hello World" };
        AZStd::ranges::subrange rangeDeduction(testString);
        AZStd::ranges::subrange rangeDeductionWithSize(testString, testString.size());
        AZStd::ranges::subrange iteratorDeduction(testString.begin(), testString.end());
        AZStd::ranges::subrange iteratorDeductionWithSize(testString.begin(), testString.end(),
            testString.size());
        EXPECT_TRUE(rangeDeduction);
        EXPECT_TRUE(rangeDeductionWithSize);
        EXPECT_TRUE(iteratorDeduction);
        EXPECT_TRUE(iteratorDeductionWithSize);
    }

    TEST_F(RangesViewTestFixture, Subrange_CanTakeSubsetOfContainer_Succeeds)
    {
        AZStd::vector<int> testVector{ 1, 3, 5, 6, 7, 6, 89, -178 };
        AZStd::ranges::subrange subVector(testVector);
        EXPECT_TRUE(subVector);
        ASSERT_FALSE(subVector.empty());
        EXPECT_EQ(testVector.data(), subVector.data());
        EXPECT_EQ(testVector.begin(), subVector.begin());
        EXPECT_EQ(testVector.end(), subVector.end());
        EXPECT_EQ(testVector.size(), subVector.size());
        EXPECT_EQ(testVector[0], subVector[0]);
        EXPECT_EQ(testVector.front(), subVector.front());
        EXPECT_EQ(testVector.back(), subVector.back());

        // Now validate the iterator operations
        subVector.advance(2);
        subVector = subVector.prev();
        subVector.advance(2);
        subVector = subVector.next();
        subVector.advance(-4);
        EXPECT_EQ(testVector.begin(), subVector.begin());

        // Obtain a copy of the subrange with the first and last elements removed
        AZStd::ranges::subrange subVectorSplice(subVector.begin() + 1, subVector.end() - 1);
        EXPECT_TRUE(subVectorSplice);
        ASSERT_FALSE(subVector.empty());
        EXPECT_LT(testVector.data(), subVectorSplice.data());
        EXPECT_EQ(testVector.begin() + 1, subVectorSplice.begin());
        EXPECT_EQ(testVector.end() - 1, subVectorSplice.end());
        ASSERT_EQ(testVector.size() - 2, subVectorSplice.size());
        EXPECT_EQ(testVector[1], subVectorSplice.front());
        EXPECT_EQ(testVector[testVector.size() - 2], subVectorSplice.back());
        EXPECT_NE(testVector.front(), subVectorSplice.front());
        EXPECT_NE(testVector.back(), subVectorSplice.back());


    }

    TEST_F(RangesViewTestFixture, EmptyView_ReturnsEmptyViewRange_Succeeds)
    {
        AZStd::ranges::empty_view<int> emptyView;

        EXPECT_EQ(nullptr, emptyView.data());
        EXPECT_EQ(0, emptyView.size());
        EXPECT_TRUE(emptyView.empty());
        EXPECT_EQ(emptyView.end(), emptyView.begin());

        EXPECT_EQ(nullptr, AZStd::ranges::views::empty<AZStd::string_view>.data());
        EXPECT_EQ(0, AZStd::ranges::views::empty<AZStd::string_view>.size());
        EXPECT_TRUE(AZStd::ranges::views::empty<AZStd::string_view>.empty());
        EXPECT_EQ(AZStd::ranges::views::empty<AZStd::string_view>.end(), AZStd::ranges::views::empty<AZStd::string_view>.begin());
    }

    TEST_F(RangesViewTestFixture, SingleView_ReturnsViewOverSingleElement_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        AZStd::ranges::single_view singleView{ AZStd::move(testString) };
        EXPECT_NE(nullptr, singleView.data());
        EXPECT_EQ(1, singleView.size());
        EXPECT_FALSE(singleView.empty());
        ASSERT_NE(singleView.end(), singleView.begin());

        auto singleViewStringIt = singleView.begin();
        EXPECT_EQ("Hello World", *singleViewStringIt);
    }

    TEST_F(RangesViewTestFixture, RefView_CanWrapStringView_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        AZStd::ranges::ref_view refView(testString);
        EXPECT_EQ(testString.size(), refView.size());
        EXPECT_EQ(testString.data(), refView.data());
        EXPECT_EQ(testString.begin(), refView.begin());
        EXPECT_EQ(testString.end(), refView.end());
        EXPECT_EQ(testString.empty(), refView.empty());
        EXPECT_EQ(testString.front(), refView.front());
        EXPECT_EQ(testString.back(), refView.back());
        EXPECT_EQ(testString[5], refView[5]);
    }

    TEST_F(RangesViewTestFixture, OwningView_CanWrapStringView_Succeeds)
    {
        AZStd::string_view sourceView{ "Hello World" };
        AZStd::string_view testString{ sourceView };
        AZStd::ranges::owning_view owningView(AZStd::move(testString));
        EXPECT_TRUE(testString.empty());
        EXPECT_FALSE(owningView.empty());
        EXPECT_EQ(sourceView.size(), owningView.size());
        EXPECT_EQ(sourceView.data(), owningView.data());
        EXPECT_EQ(sourceView.begin(), owningView.begin());
        EXPECT_EQ(sourceView.end(), owningView.end());
        EXPECT_EQ(sourceView.empty(), owningView.empty());
        EXPECT_EQ(sourceView.front(), owningView.front());
        EXPECT_EQ(sourceView.back(), owningView.back());
        EXPECT_EQ(sourceView[5], owningView[5]);
    }

    MATCHER_P(ZipViewAtSentinel, sentinel, "") {
        *result_listener << "zip view has iterated to sentinel";
        return !(arg == sentinel);
    }
    TEST_F(RangesViewTestFixture, ZipView_CompilesWithRange_Succeeds)
    {
        AZStd::string_view sourceView{ "Hello World" };
        AZStd::ranges::zip_view zipView(AZStd::move(sourceView));
        auto zipItTuple = zipView.begin();
        auto zipSentinelTuple = zipView.end();
        ASSERT_THAT(zipItTuple, ZipViewAtSentinel(zipSentinelTuple));
        EXPECT_EQ('H', AZStd::get<0>(*zipItTuple));
        ptrdiff_t zipDistance = zipSentinelTuple - zipItTuple;
        EXPECT_EQ(11, zipDistance);

        AZStd::list<int> sourceList{ 1, 2, 3, 4, 5 };
        AZStd::ranges::zip_view zipView2(AZStd::move(sourceList));
        auto zipListTupleIt = zipView2.begin();
        auto zipListTupleEnd = zipView2.end();
        ASSERT_THAT(zipListTupleIt, ZipViewAtSentinel(zipListTupleEnd));
    }

    TEST_F(RangesViewTestFixture, ZipView_CanIteratOverMultipleContainers_Succeeds)
    {
        AZStd::string_view sourceView{ "abcdef" };
        AZStd::vector intVector{ 1, 2, 3, 4, 5 };
        AZStd::list<uint32_t> uintList{ 2, 4, 6, 8, 10 };
        constexpr int expectedIterations = 5;

        int iterationCount{};
        for (auto [charX, intY, uintZ] : (AZStd::ranges::views::zip(sourceView, AZStd::move(intVector), AZStd::move(uintList))))
        {
            ++iterationCount;
            switch (charX)
            {
            case 'a':
                EXPECT_EQ(1, intY);
                EXPECT_EQ(2, uintZ);
                break;
            case 'b':
                EXPECT_EQ(2, intY);
                EXPECT_EQ(4, uintZ);
                break;
            case 'c':
                EXPECT_EQ(3, intY);
                EXPECT_EQ(6, uintZ);
                break;
            case 'd':
                EXPECT_EQ(4, intY);
                EXPECT_EQ(8, uintZ);
                break;
            case 'e':
                EXPECT_EQ(5, intY);
                EXPECT_EQ(10, uintZ);
                break;
            default:
                ADD_FAILURE() << "Unexpected character value " << charX << " found when iterating zip view";
            }
        }

        EXPECT_EQ(expectedIterations, iterationCount);
    }

    TEST_F(RangesViewTestFixture, SplitView_CanSplitPatterns_Succeeds)
    {
        AZStd::string_view emptyView{ "" };
        auto splitView = AZStd::ranges::views::split(emptyView, " ");
        auto splitIt = splitView.begin();
        EXPECT_EQ(splitView.end(), splitIt);

        AZStd::string_view testView1{ "Hello" };
        auto splitViewCharPattern = AZStd::ranges::views::split(testView1, ' ');
        auto splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);

        AZStd::string_view testView2{ "Hello World" };
        splitViewCharPattern = AZStd::ranges::views::split(testView2, ' ');
        splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("World", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);

        AZStd::string_view testView3{ "Hello World Moon" };
        splitViewCharPattern = AZStd::ranges::views::split(testView3, ' ');
        splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("World", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Moon", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);


        AZStd::string_view testView4{ "Hello World Moon " };
        splitViewCharPattern = AZStd::ranges::views::split(testView4, ' ');
        splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("World", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Moon", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);

        AZStd::string_view testView5{ "Hello World Moon  " };
        splitViewCharPattern = AZStd::ranges::views::split(testView5, ' ');
        splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("World", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Moon", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);

        AZStd::string_view testView6{ "Hello  World Moon" };
        splitViewCharPattern = AZStd::ranges::views::split(testView6, ' ');
        splitCharIt = splitViewCharPattern.begin();
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Hello", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("World", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        ASSERT_NE(splitViewCharPattern.end(), splitCharIt);
        EXPECT_EQ("Moon", AZStd::string_view(*splitCharIt));
        ++splitCharIt;
        EXPECT_EQ(splitViewCharPattern.end(), splitCharIt);
    }

    TEST_F(RangesViewTestFixture, SplitView_SplitsFromNonString_Succeeds)
    {
        const AZStd::vector<int> testVector{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        // Split the vector on 3
        auto splitView = testVector | AZStd::ranges::views::split(3);
        auto splitIt = splitView.begin();
        ASSERT_NE(splitView.end(), splitIt);
        auto splitSubrange = *splitIt;
        {
            AZStd::array expectedValue{ 1, 2 };
            EXPECT_TRUE(AZStd::ranges::equal(expectedValue, splitSubrange));
        }

        ++splitIt;
        ASSERT_NE(splitView.end(), splitIt);
        splitSubrange = *splitIt;
        {
            AZStd::array expectedValue{ 4, 5, 6, 7, 8, 9, 10 };
            EXPECT_TRUE(AZStd::ranges::equal(expectedValue, splitSubrange));
        }
    }

    TEST_F(RangesViewTestFixture, JoinView_IteratesOverInnerViews_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "HelloWorldMoonSun";
        using Rope = AZStd::fixed_vector<AZStd::string_view, 32>;
        Rope rope{ "Hello", "World", "Moon", "Sun" };
        AZStd::fixed_string<128> accumString;
        for (auto&& charElement : AZStd::ranges::join_view(rope))
        {
            accumString.push_back(charElement);
        }

        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, JoinView_IteratesCanIterateOverSplitView_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "HelloWorldMoonSun";
        constexpr AZStd::string_view splitExpression = "Hello,World,Moon,Sun";
        AZStd::fixed_string<128> accumString;

        for (auto&& charElement : AZStd::ranges::views::join(AZStd::ranges::views::split(splitExpression, ',')))
        {
            accumString.push_back(charElement);
        }

        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, JoinView_IterSwapCustomization_Succeeds)
    {
        AZStd::fixed_vector<AZStd::string, 8> testVector1{ "World", "Hello" };
        AZStd::fixed_vector<AZStd::string, 8> testVector2{ "Value", "First" };
        auto joinView1 = AZStd::ranges::views::join(testVector1);
        auto joinView2 = AZStd::ranges::views::join(testVector2);
        auto joinViewIter1 = joinView1.begin();
        auto joinViewIter2 = joinView2.begin();
        // swaps the 'W' and 'V'
        AZStd::ranges::iter_swap(joinViewIter1, joinViewIter2);

        // swaps the 'H' and 'F' of the second string of each vector

        /* Commented in out original AZStd::ranges::advanceand the second AZStd::ranges::iter_swap cll
        AZStd::ranges::advance(joinViewIter1, 5, joinView1.end());
        AZStd::ranges::advance(joinViewIter2, 5, joinView2.end());
        AZStd::ranges::iter_swap(joinViewIter1, joinViewIter2);
        //
        // There is a bug in MSVC compiler when swapping a char& that occurs only in profile configuration
        // The AZStd::ranges::iter_swap eventually calls AZStd::ranges::swap which should swap 5th characters
        // of each vector.
        // But instead the testVector2[5] gets the correct character of testVector1[5] `H` swapped to it.
        // But the testVector1[5] seems to get the character from testVector2[0] which is now 'V' swapped to it
        //
        // I believe the MSVC is probably performing a bad optimization where it comes to the read address
        // of the *joinViewIter2(char&) iterator
        //
        // The workaround that is working is to use AZStd::ranges::next to create a new join_view::iterator
        // and perform AZStd::ranges::iter_swap on those objects.
        */
        AZStd::ranges::iter_swap(AZStd::ranges::next(joinViewIter1, 5, joinView1.end()), AZStd::ranges::next(joinViewIter2, 5, joinView2.end()));
        EXPECT_EQ("Vorld", testVector1[0]);
        EXPECT_EQ("Fello", testVector1[1]);
        EXPECT_EQ("Walue", testVector2[0]);
        EXPECT_EQ("Hirst", testVector2[1]);
    }

    TEST_F(RangesViewTestFixture, JoinView_IterMoveCustomization_Succeeds)
    {
        using StringWrapper = AZStd::ranges::single_view<AZStd::string>;

        AZStd::fixed_vector<StringWrapper, 8> testVector1{ StringWrapper{"5"}, StringWrapper{"10"} };
        AZStd::fixed_vector<StringWrapper, 8> testVector2{ StringWrapper{"15"}, StringWrapper{"20"} };
        auto joinView1 = AZStd::ranges::views::join(testVector1);
        auto joinView2 = AZStd::ranges::views::join(testVector2);
        auto joinViewIter1 = joinView1.begin();
        auto joinViewIter2 = joinView2.begin();
        AZStd::string value = AZStd::ranges::iter_move(joinViewIter1);

        EXPECT_EQ("5", value);
        EXPECT_TRUE((*joinViewIter1).empty());
        ++joinViewIter2;
        value = AZStd::ranges::iter_move(joinViewIter2);
        EXPECT_EQ("20", value);
        EXPECT_TRUE((*joinViewIter2).empty());
    }

    TEST_F(RangesViewTestFixture, JoinView_ReverseIterationOverRangeOfRanges_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "nuSnooMdlroWolleH";
        using Rope = AZStd::fixed_vector<AZStd::string_view, 32>;
        Rope rope{ "Hello", "World", "Moon", "Sun" };
        AZStd::fixed_string<128> accumString;

        auto joinView = AZStd::ranges::views::join(rope);
        // Iterate over view in reverse(can replace normal for loop with range based one, once AZStd::ranges::reverse_view is available)
        for (auto revIter = AZStd::ranges::rbegin(joinView); revIter != AZStd::ranges::rend(joinView); ++revIter)
        {
            accumString.push_back(*revIter);
        }

        EXPECT_EQ(expectedString, accumString);
    }

    // join_with_view
    TEST_F(RangesViewTestFixture, JoinWithView_IteratesOverRangeOfRangesWithSeparator_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "Hello, World, Moon, Sun";
        using RopeWithSeparator = AZStd::fixed_vector<AZStd::string_view, 32>;
        RopeWithSeparator rope{ "Hello", "World", "Moon", "Sun" };
        AZStd::fixed_string<128> accumString;
        // Protip: Do not use a string literal directly with join_with
        // A string literal is actually a reference to a C array that includes the null-terminator character
        // Convert it to a string_view
        using namespace AZStd::literals::string_view_literals;
        for (auto&& charElement : AZStd::ranges::views::join_with(rope, ", "_sv))
        {
            accumString.push_back(charElement);
        }

        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, JoinWithView_IteratesCanIterateOverSplitView_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "Hello World Moon Sun";
        constexpr AZStd::string_view splitExpression = "Hello,World,Moon,Sun";
        {
            // Test range adaptor with char literal
            AZStd::fixed_string<128> accumString;

            for (auto&& charElement : splitExpression | AZStd::ranges::views::split(',') | AZStd::ranges::views::join_with(' '))
            {
                accumString.push_back(charElement);
            }

            EXPECT_EQ(expectedString, accumString);
        }
        {
            // Test range adaptor with string_view
            // DO NOT use string literal as it is deduced as an array that includes the NUL character
            // as part of the range
            AZStd::fixed_string<128> accumString;
            using namespace AZStd::literals::string_view_literals;
            for (auto&& charElement : splitExpression | AZStd::ranges::views::split(',') | AZStd::ranges::views::join_with(" "_sv))
            {
                accumString.push_back(charElement);
            }

            EXPECT_EQ(expectedString, accumString);
        }
    }

    TEST_F(RangesViewTestFixture, JoinWithView_ReverseIterationOverRangeOfRanges_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "nuS ,nooM ,dlroW ,olleH";
        using RopeWithSeparator = AZStd::fixed_vector<AZStd::string_view, 32>;
        RopeWithSeparator rope{ "Hello", "World", "Moon", "Sun" };
        AZStd::fixed_string<128> accumString;

        using namespace AZStd::literals::string_view_literals;
        auto joinWithView = AZStd::ranges::views::join_with(rope, ", "_sv);
        // Iterate over view in reverse(can replace normal for loop with range based one, once AZStd::ranges::reverse_view is available)
        for (auto revIter = AZStd::ranges::rbegin(joinWithView); revIter != AZStd::ranges::rend(joinWithView); ++revIter)
        {
            accumString.push_back(*revIter);
        }

        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, JoinWithView_IsConstexpr_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "Hello, World, Moon, Sun";
        constexpr AZStd::array<AZStd::string_view, 4> rope{ "Hello", "World", "Moon", "Sun" };
        using namespace AZStd::literals::string_view_literals;
        constexpr AZStd::fixed_string<128> accumString(AZStd::from_range, rope | AZStd::views::join_with(", "_sv));
        static_assert(accumString == expectedString);
    }

    // elements_view
    TEST_F(RangesViewTestFixture, ElementsView_CanIterateVectorOfTuple_Succeeds)
    {
        using TestTuple = AZStd::tuple<int, AZStd::string, bool>;
        AZStd::vector testVector{ TestTuple{1, "hello", false}, TestTuple{2, "world", true},
            TestTuple{3, "Moon", false} };

        auto firstElementView = AZStd::ranges::views::elements<0>(testVector);
        auto firstElementBegin = AZStd::ranges::begin(firstElementView);
        auto firstElementEnd = AZStd::ranges::end(firstElementView);
        EXPECT_NE(firstElementEnd, firstElementBegin);
        ASSERT_EQ(3, firstElementView.size());
        EXPECT_EQ(1, firstElementView[0]);
        EXPECT_EQ(2, firstElementView[1]);
        EXPECT_EQ(3, firstElementView[2]);

        auto secondElementView = AZStd::ranges::views::elements<1>(testVector);
        ASSERT_EQ(3, secondElementView.size());
        EXPECT_EQ("hello", secondElementView[0]);
        EXPECT_EQ("world", secondElementView[1]);
        EXPECT_EQ("Moon", secondElementView[2]);

        auto thirdElementView = AZStd::ranges::views::elements<2>(testVector);
        ASSERT_EQ(3, thirdElementView.size());
        EXPECT_FALSE(thirdElementView[0]);
        EXPECT_TRUE(thirdElementView[1]);
        EXPECT_FALSE(thirdElementView[2]);

        using TestPair = AZStd::pair<AZStd::string, int>;
        AZStd::vector testPairVector{ TestPair{"hello", 5}, TestPair{"world", 10}, TestPair{"Sun", 15} };
        using ElementsViewBase = AZStd::ranges::views::all_t<decltype((testPairVector))>;
        using ElementsViewType = AZStd::ranges::elements_view<ElementsViewBase, 0>;

        constexpr AZStd::string_view expectedString = "helloworldSun";
        AZStd::string accumString;
        for (auto&& stringValue : ElementsViewType(testPairVector))
        {
            accumString += stringValue;
        }

        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, ElementsView_KeysAlias_CanIterateAssociativeContainerKeyType)
    {
        using PairType = AZStd::pair<int, const char*>;
        AZStd::map testMap{ PairType{1, "Hello"}, PairType{2, "World"}, PairType{3, "Sun"}, PairType{45, "RandomText"} };

        int accumResult{};
        for (int key : AZStd::ranges::views::keys(testMap))
        {
            accumResult += key;
        }

        EXPECT_EQ(51, accumResult);
    }

    TEST_F(RangesViewTestFixture, ElementsView_ValuesAlias_CanIterateAssociativeContainerMappedType)
    {
        using PairType = AZStd::pair<int, const char*>;
        AZStd::map testMap{ PairType{1, "Hello"}, PairType{2, "World"}, PairType{3, "Sun"}, PairType{45, "RandomText"} };

        AZStd::string accumResult{};
        for (const char* value : AZStd::ranges::views::values(testMap))
        {
            accumResult += value;
        }

        EXPECT_EQ("HelloWorldSunRandomText", accumResult);
    }

    TEST_F(RangesViewTestFixture, ElementsView_ReverseIteration_Succeeds)
    {
        using PairType = AZStd::pair<int, const char*>;
        AZStd::map testMap{ PairType{1, "Hello"}, PairType{2, "World"}, PairType{3, "Sun"}, PairType{45, "RandomText"} };

        AZStd::string accumResult{};
        auto valuesView = AZStd::ranges::views::values(testMap);
        for (auto revIter = AZStd::ranges::rbegin(valuesView); revIter != AZStd::ranges::rend(valuesView); ++revIter)
        {
            accumResult += *revIter;
        }

        EXPECT_EQ("RandomTextSunWorldHello", accumResult);
    }

    TEST_F(RangesViewTestFixture, TransformView_TransformStringArrayOfNumbers_ToIntView_Succeeds)
    {
        constexpr int expectedResult = 1 + 2 + 3;
        AZStd::vector stringArray{ AZStd::string("1"), AZStd::string("2"), AZStd::string("3") };

        auto StringToInt = [](const AZStd::string& numString) -> int
        {
            constexpr int base = 10;
            return static_cast<int>(strtoll(numString.c_str(), nullptr, base));
        };

        int accumResult{};
        for (int value : stringArray | AZStd::ranges::views::transform(StringToInt))
        {
            accumResult += value;
        }

        EXPECT_EQ(expectedResult, accumResult);
    }

    TEST_F(RangesViewTestFixture, TransformView_GetMemberFromRangeElement_Succeeds)
    {
        struct IntWrapper
        {
            int m_value{};
        };
        constexpr int expectedResult = 1 + 2 + 3;
        AZStd::vector testArray{ IntWrapper{ 1 }, IntWrapper{ 2 }, IntWrapper{ 3 } };

        auto GetValueMember = [](const IntWrapper& wrapper) -> decltype(auto)
        {
            return wrapper.m_value;
        };

        int accumResult{};
        for (int value : testArray | AZStd::ranges::views::transform(GetValueMember))
        {
            accumResult += value;
        }

        EXPECT_EQ(expectedResult, accumResult);
    }

    TEST_F(RangesViewTestFixture, CommonView_CanPassDifferentIteratorAndSentinelTypes_ToInsertFunction)
    {
        constexpr AZStd::string_view expectedString = "Hello,World,Moon,Sun";
        static constexpr auto arrayOfLiterals{ AZStd::to_array<AZStd::string_view>({"Hello", "World", "Moon", "Sun"}) };
        auto commonView = AZStd::ranges::views::common(AZStd::ranges::views::join_with(arrayOfLiterals, ','));

        AZStd::fixed_string<128> accumString{ commonView.begin(), commonView.end() };
        EXPECT_EQ(expectedString, accumString);
    }

    TEST_F(RangesViewTestFixture, FilterView_CanFilterWhiteSpaceFromString_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "Hello,World,Moon,Sun";
        constexpr AZStd::string_view testString = "Hello, World, Moon, Sun";

        AZStd::string resultString;
        for (char elem : testString | AZStd::ranges::views::filter([](char element) { return !::isspace(element); }))
        {
            resultString += elem;
        }
        EXPECT_EQ(expectedString, resultString);
    }

    TEST_F(RangesViewTestFixture, FilterView_CanIterateBidirectionalRangeInReverse_Succeeds)
    {
        constexpr AZStd::string_view expectedString = "nuS,nooM,dlroW,olleH";
        constexpr AZStd::string_view testString = "Hello, World, Moon, Sun";

        AZStd::ranges::filter_view testFilterView(testString, [](char element) { return !::isspace(element); });
        AZStd::string resultString;
        for (auto it = AZStd::ranges::rbegin(testFilterView); it != AZStd::ranges::rend(testFilterView); ++it)
        {
            resultString += *it;
        }
        EXPECT_EQ(expectedString, resultString);
    }

    TEST_F(RangesViewTestFixture, FilterView_CanAccessPredicate)
    {
        const AZStd::ranges::filter_view testFilterView("", [](char element) { return !::isspace(element); });
        const auto& filterViewPredicate = testFilterView.pred();
        EXPECT_TRUE(filterViewPredicate('a'));
        EXPECT_FALSE(filterViewPredicate(' '));
        EXPECT_FALSE(filterViewPredicate('\n'));
    }

    TEST_F(RangesViewTestFixture, ReverseView_CanIterateOverBidirectionalRange)
    {
        constexpr AZStd::string_view expectedString = "nuS,nooM,dlroW,olleH";
        constexpr AZStd::string_view testString = "Hello,World,Moon,Sun";

        const AZStd::ranges::reverse_view testReverseView(testString);
        EXPECT_EQ(testString.size(), testReverseView.size());

        AZStd::string resultString{ testReverseView.begin(), testReverseView.end() };
        EXPECT_EQ(expectedString, resultString);
    }

    TEST_F(RangesViewTestFixture, ReverseView_ReverseOfReverse_ReturnsOriginalView)
    {
        constexpr AZStd::string_view testString = "Hello,World,Moon,Sun";

        AZStd::string resultString = AZStd::views::reverse(testString) | AZStd::views::reverse;
        EXPECT_EQ(testString, resultString);
    }

    TEST_F(RangesViewTestFixture, ReverseView_SubrangeOfReverseIterators_ReturnsSubrangeOfOriginalIterators)
    {
        constexpr AZStd::string_view testString = "Hello,World,Moon,Sun";

        using namespace AZStd::literals::string_view_literals;
        // form a subrange of reverse iterators to "World"
        auto testSubrange = AZStd::ranges::subrange(testString.begin() + 6, testString.begin() + 11);
        EXPECT_TRUE(AZStd::ranges::equal(testSubrange, "World"_sv));

        auto testSubrangeOfReverse = testSubrange | AZStd::views::reverse;
        EXPECT_TRUE(AZStd::ranges::equal(testSubrangeOfReverse, "dlroW"_sv));

        auto testSubrangeOfReverseReverse = testSubrangeOfReverse | AZStd::views::reverse;
        static_assert(AZStd::same_as<decltype(testSubrange), decltype(testSubrangeOfReverseReverse)>);
        EXPECT_TRUE(AZStd::ranges::equal(testSubrangeOfReverseReverse, "World"_sv));
    }

    TEST_F(RangesViewTestFixture, CountedView_CanCreateSubrange_FromIteratorAndCounted)
    {
        using namespace AZStd::literals::string_view_literals;

        constexpr AZStd::string_view testString = "Hello,World,Moon,Sun";
        constexpr auto testView = AZStd::views::counted(testString.begin() + 6, 5);
        EXPECT_TRUE(AZStd::ranges::equal(testView, "World"_sv));
    }

    TEST_F(RangesViewTestFixture, CountedView_CanCreateSubrange_FromNonContiguousIterator)
    {
        using namespace AZStd::literals::string_view_literals;

        AZStd::list<int> testContainer{ 1, 2, 3, 4, 5 };
        auto testView = AZStd::views::counted(testContainer.begin(), 3);
        constexpr auto expectedResult = AZStd::to_array<int>({ 1, 2, 3 });
        EXPECT_TRUE(AZStd::ranges::equal(testView, expectedResult));
    }

    TEST_F(RangesViewTestFixture, AsRvalueView_MovesContainerElements_Succeeds)
    {
        using namespace AZStd::literals::string_view_literals;

        AZStd::vector testStringContainer{ AZStd::string("Hello"), AZStd::string("World") };
        AZStd::list movedStringContainer{ AZStd::from_range, testStringContainer | AZStd::views::as_rvalue };

        EXPECT_THAT(movedStringContainer, ::testing::ElementsAre(AZStd::string_view("Hello"), AZStd::string_view("World")));
        EXPECT_THAT(testStringContainer, ::testing::ElementsAre(AZStd::string_view(), AZStd::string_view()));
    }

    namespace RangesViewTestInternal
    {
        struct ConstMutableContainer
        {
            struct iterator
            {
                using value_type = int;
                using iterator_concept = AZStd::bidirectional_iterator_tag;
                using iterator_category = AZStd::bidirectional_iterator_tag;

                const int& operator*() const { return m_charElement; }
                int& operator*() { return m_intElement; }
                const int* operator->() const { return &m_charElement; }
                int* operator->() { return &m_intElement; }
                iterator& operator++() { ++m_intElement; return *this; }
                iterator operator++(int) { iterator tmp(*this); ++m_intElement; return tmp; }
                iterator& operator--() { --m_intElement; return *this; }
                iterator operator--(int) { iterator tmp(*this); --m_intElement; return tmp; }
                bool operator==(const iterator& y) const { return m_intElement == y.m_intElement; }
                bool operator!=(const iterator& y) const { return !operator==(y); }
                ptrdiff_t operator-(const iterator& y) const { return m_intElement - y.m_intElement; }

                int m_charElement = 'A';
                int m_intElement = 55;
            };

            using iterator = iterator;
            using const_iterator = const iterator;

            iterator begin() { return iterator{}; }
            iterator begin() const { return iterator{}; }
            iterator end() { return iterator{ 'A', 60 }; }
            iterator end() const { return iterator{ 'A', 60 }; }
        };
    }

    TEST_F(RangesViewTestFixture, AsConstView_ActAsConstantViewOfContainerSucceeds)
    {
        RangesViewTestInternal::ConstMutableContainer testContainer;
        AZStd::ranges::iterator_t<RangesViewTestInternal::ConstMutableContainer> foundIter = testContainer.begin();
        EXPECT_EQ(55, *foundIter);

        auto constView = testContainer | AZStd::views::as_const;
        ASSERT_NE(constView.end(), constView.begin());
        for (auto elem : constView)
        {
            EXPECT_EQ('A', elem);
        }

        AZStd::string_view testString;
        [[maybe_unused]] auto constStringView = testString | AZStd::views::as_const;
        static_assert(AZStd::same_as<decltype(testString), decltype(constStringView)>);
    }

    TEST_F(RangesViewTestFixture, IotaView_CanGenerateRangeUpToBound)
    {
        constexpr AZStd::array expectedValues{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        AZStd::vector testValue(AZStd::from_range, AZStd::views::iota(0, 10));
        EXPECT_THAT(testValue, ::testing::ElementsAreArray(expectedValues));
    }

    TEST_F(RangesViewTestFixture, IotaView_CanGenerateUnboundedValues)
    {
        constexpr AZStd::array expectedValues{ -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

        constexpr int breakValue = 11;
        AZStd::vector<int> resultValues;
        for (int i : AZStd::views::iota(-3))
        {
            resultValues.push_back(i);
            if (i >= breakValue)
            {
                break;
            }
        }
        EXPECT_THAT(resultValues, ::testing::ElementsAreArray(expectedValues));
    }

    TEST_F(RangesViewTestFixture, IotaView_WithZipView_CanGenerateIndexForEveryElementOfOtherView)
    {
        const AZStd::map expectedValues{ AZStd::pair{0, 'H'}, {1, 'e'}, { 2, 'l'}, { 3, 'l'}, {4, 'o'} };
        constexpr AZStd::string_view testString = "Hello";

        AZStd::map<int, char> resultValues;
        for (auto [i, elem] : AZStd::views::zip(AZStd::views::iota(0), testString))
        {
            resultValues[static_cast<int>(i)] = elem;
        }

        EXPECT_THAT(resultValues, ::testing::ElementsAreArray(expectedValues));
    }

    TEST_F(RangesViewTestFixture, RepeatView_CanGenerateRangeUpToBound)
    {
        using namespace AZStd::literals::string_view_literals;
        constexpr AZStd::array expectedValues{ "Hello"_sv, "Hello"_sv, "Hello"_sv };
        AZStd::vector testValue(AZStd::from_range, AZStd::views::repeat("Hello"_sv, 3));
        EXPECT_THAT(testValue, ::testing::ElementsAreArray(expectedValues));
    }

    TEST_F(RangesViewTestFixture, RepeatView_CanGenerateUnboundedValues)
    {
        using namespace AZStd::literals::string_view_literals;
        constexpr int maxIterations = 4;
        constexpr AZStd::array<AZStd::string_view, maxIterations> expectedValues{ "Hello"_sv, "Hello"_sv, "Hello"_sv, "Hello"_sv };

        AZStd::vector<AZStd::string_view> resultValues;
        int i = 0;
        for (AZStd::string_view testView : AZStd::views::repeat("Hello"_sv))
        {
            if (i++ >= maxIterations)
            {
                break;
            }
            resultValues.push_back(testView);
        }
        EXPECT_THAT(resultValues, ::testing::ElementsAreArray(expectedValues));
    }
}

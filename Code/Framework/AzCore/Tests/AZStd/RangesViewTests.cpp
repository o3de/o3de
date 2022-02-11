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
#include <AzCore/std/ranges/elements_view.h>
#include <AzCore/std/ranges/empty_view.h>
#include <AzCore/std/ranges/join_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/ranges/single_view.h>
#include <AzCore/std/ranges/split_view.h>
#include <AzCore/std/ranges/subrange.h>
#include <AzCore/std/ranges/zip_view.h>
#include <AzCore/std/string/string_view.h>

namespace UnitTest
{
    class RangesViewTestFixture
        : public ScopedAllocatorSetupFixture
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
        auto splitView = AZStd::ranges::views::split(testVector, 3);
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
        AZStd::ranges::advance(joinViewIter1, 5, joinView1.end());
        AZStd::ranges::advance(joinViewIter2, 5, joinView2.end());
        // swaps the 'H' and 'F' of the second string of each vector
        AZStd::ranges::iter_swap(joinViewIter1, joinViewIter2);
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
}

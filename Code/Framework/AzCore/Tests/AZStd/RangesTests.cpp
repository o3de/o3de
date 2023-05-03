/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/ranges/ranges.h>

namespace UnitTest
{
    class RangesTestFixture
        : public LeakDetectionFixture
    {};

    struct RangeLikeCustomizationPoint {};

    RangeLikeCustomizationPoint* begin(RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    RangeLikeCustomizationPoint* end(RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    const RangeLikeCustomizationPoint* begin(const RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    const RangeLikeCustomizationPoint* end(const RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    RangeLikeCustomizationPoint* rbegin(RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    RangeLikeCustomizationPoint* rend(RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    const RangeLikeCustomizationPoint* rbegin(const RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }
    const RangeLikeCustomizationPoint* rend(const RangeLikeCustomizationPoint& rangeLike)
    {
        return &rangeLike;
    }

    constexpr size_t size(const RangeLikeCustomizationPoint&)
    {
        return 0;
    }
    constexpr size_t size(RangeLikeCustomizationPoint&)
    {
        return 0;
    }


    // range access
    TEST_F(RangesTestFixture, RangesBegin_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 0, AZStd::ranges::begin(extentArray));
    }

    TEST_F(RangesTestFixture, RangesBegin_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::begin), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesBegin_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.begin(), AZStd::ranges::begin(strView));
    }

    TEST_F(RangesTestFixture, RangesBegin_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::begin(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesEnd_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 5, AZStd::ranges::end(extentArray));
    }

    TEST_F(RangesTestFixture, RangesEnd_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::end), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesEnd_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.end(), AZStd::ranges::end(strView));
    }
    TEST_F(RangesTestFixture, RangesEnd_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::end(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesCBegin_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 0, AZStd::ranges::cbegin(extentArray));
    }

    TEST_F(RangesTestFixture, RangesCBegin_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.cbegin(), AZStd::ranges::cbegin(strView));
    }

    TEST_F(RangesTestFixture, RangesCBegin_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::cbegin(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesCEnd_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 5, AZStd::ranges::cend(extentArray));
    }

    TEST_F(RangesTestFixture, RangesCEnd_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.cend(), AZStd::ranges::cend(strView));
    }

    TEST_F(RangesTestFixture, RangesCEnd_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::cend(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesRBegin_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 5, AZStd::ranges::rbegin(extentArray).base());
    }

    TEST_F(RangesTestFixture, RangesRBegin_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.rbegin(), AZStd::ranges::rbegin(strView));
    }
    TEST_F(RangesTestFixture, RangesRBegin_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::rbegin(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesREnd_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray, AZStd::ranges::rend(extentArray).base());
    }

    TEST_F(RangesTestFixture, RangesREnd_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.rend(), AZStd::ranges::rend(strView));
    }
    TEST_F(RangesTestFixture, RangesREnd_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::rend(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesCRBegin_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 5, AZStd::ranges::crbegin(extentArray).base());
    }

    TEST_F(RangesTestFixture, RangesCRBegin_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.crbegin(), AZStd::ranges::crbegin(strView));
    }
    TEST_F(RangesTestFixture, RangesCRBegin_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::crbegin(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesCREnd_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray + 0, AZStd::ranges::crend(extentArray).base());
    }

    TEST_F(RangesTestFixture, RangesCREnd_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.crend(), AZStd::ranges::crend(strView));
    }

    TEST_F(RangesTestFixture, RangesCREnd_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        EXPECT_EQ(&rangeLike, AZStd::ranges::crend(rangeLike));
    }

    // range access - size
    TEST_F(RangesTestFixture, RangesSize_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        constexpr ArrayExtentType extentArray{};
        static_assert(5 == AZStd::ranges::size(extentArray));
    }

    TEST_F(RangesTestFixture, RangesSize_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::size), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesSize_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        EXPECT_EQ(strView.size(), AZStd::ranges::size(strView));
    }


    TEST_F(RangesTestFixture, RangesSize_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;

        EXPECT_EQ(0, AZStd::ranges::size(rangeLike));
    }

    TEST_F(RangesTestFixture, RangesSSize_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        constexpr ArrayExtentType extentArray{};
        static_assert(AZStd::signed_integral<decltype(AZStd::ranges::ssize(extentArray))>);
        static_assert(5 == AZStd::ranges::ssize(extentArray));
    }

    TEST_F(RangesTestFixture, RangesSSize_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::ssize), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesSSize_Compiles_WithMemberOverload)
    {
        AZStd::string_view strView;
        static_assert(AZStd::signed_integral<decltype(AZStd::ranges::ssize(strView))>);
        EXPECT_EQ(strView.size(), AZStd::ranges::ssize(strView));
    }

    TEST_F(RangesTestFixture, RangesSSize_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;
        static_assert(AZStd::signed_integral<decltype(AZStd::ranges::ssize(rangeLike))>);
        EXPECT_EQ(0, AZStd::ranges::ssize(rangeLike));
    }

    // range access - empty
    TEST_F(RangesTestFixture, RangesEmpty_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        constexpr ArrayExtentType extentArray{};
        static_assert(!AZStd::ranges::empty(extentArray));
    }

    TEST_F(RangesTestFixture, RangesEmpty_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::empty), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesEmpty_Compiles_WithMemberOverload)
    {
        constexpr AZStd::string_view strView;
        static_assert(AZStd::ranges::empty(strView));
    }

    TEST_F(RangesTestFixture, RangesEmpty_Compiles_WithADL)
    {
        constexpr RangeLikeCustomizationPoint rangeLike;

        static_assert(AZStd::ranges::empty(rangeLike));
    }

    // range access - data
    TEST_F(RangesTestFixture, RangesData_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        constexpr ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray, AZStd::ranges::data(extentArray));
    }

    TEST_F(RangesTestFixture, RangesData_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::data), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesData_Compiles_WithMemberOverload)
    {
        constexpr AZStd::string_view strView;
        EXPECT_EQ(strView.data(), AZStd::ranges::data(strView));
    }

    TEST_F(RangesTestFixture, RangesData_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;

        EXPECT_EQ(&rangeLike, AZStd::ranges::data(rangeLike));
    }

    // range access - cdata
    TEST_F(RangesTestFixture, RangesCData_Compiles_WithExtentArray)
    {
        using ArrayExtentType = int[5];

        constexpr ArrayExtentType extentArray{};
        EXPECT_EQ(extentArray, AZStd::ranges::cdata(extentArray));
    }

    TEST_F(RangesTestFixture, RangesCData_DoesNotCompile_WithNoExtentArray)
    {
        using ArrayNoExtentType = int[];
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::cdata), ArrayNoExtentType>);
    }

    TEST_F(RangesTestFixture, RangesCData_Compiles_WithMemberOverload)
    {
        constexpr AZStd::string_view strView;
        EXPECT_EQ(strView.data(), AZStd::ranges::cdata(strView));
    }

    TEST_F(RangesTestFixture, RangesCData_Compiles_WithADL)
    {
        RangeLikeCustomizationPoint rangeLike;

        EXPECT_EQ(&rangeLike, AZStd::ranges::cdata(rangeLike));
    }

    // Ranges TypeTraits Test
    TEST_F(RangesTestFixture, RangesTypeTraits_Compiles)
    {
        // string_view
        static_assert(AZStd::same_as<AZStd::ranges::iterator_t<AZStd::string_view>, const char*>);
        static_assert(AZStd::same_as<AZStd::ranges::sentinel_t<AZStd::string_view>, const char*>);
        static_assert(AZStd::same_as<AZStd::ranges::range_difference_t<AZStd::string_view>, ptrdiff_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_size_t<AZStd::string_view>, size_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_value_t<AZStd::string_view>, char>);
        static_assert(AZStd::same_as<AZStd::ranges::range_reference_t<AZStd::string_view>, const char&>);
        static_assert(AZStd::same_as<AZStd::ranges::range_rvalue_reference_t<AZStd::string_view>, const char&&>);

        // string
        static_assert(AZStd::same_as<AZStd::ranges::iterator_t<AZStd::string>, char*>);
        static_assert(AZStd::same_as<AZStd::ranges::sentinel_t<AZStd::string>, char*>);
        static_assert(AZStd::same_as<AZStd::ranges::range_difference_t<AZStd::string>, ptrdiff_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_size_t<AZStd::string>, size_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_value_t<AZStd::string>, char>);
        static_assert(AZStd::same_as<AZStd::ranges::range_reference_t<AZStd::string>, char&>);
        static_assert(AZStd::same_as<AZStd::ranges::range_rvalue_reference_t<AZStd::string>, char&&>);

        // int array type
        using ArrayExtentType = int[5];
        static_assert(AZStd::same_as<AZStd::ranges::iterator_t<ArrayExtentType>, int*>);
        static_assert(AZStd::same_as<AZStd::ranges::sentinel_t<ArrayExtentType>, int*>);
        static_assert(AZStd::same_as<AZStd::ranges::range_difference_t<ArrayExtentType>, ptrdiff_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_size_t<ArrayExtentType>, size_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_value_t<ArrayExtentType>, int>);
        static_assert(AZStd::same_as<AZStd::ranges::range_reference_t<ArrayExtentType>, int&>);
        static_assert(AZStd::same_as<AZStd::ranges::range_rvalue_reference_t<ArrayExtentType>, int&&>);

        // RangeLikeCustomizationPoint type which specializes several range functions
        static_assert(AZStd::same_as<AZStd::ranges::iterator_t<RangeLikeCustomizationPoint>, RangeLikeCustomizationPoint*>);
        static_assert(AZStd::same_as<AZStd::ranges::sentinel_t<RangeLikeCustomizationPoint>, RangeLikeCustomizationPoint*>);
        static_assert(AZStd::same_as<AZStd::ranges::range_difference_t<RangeLikeCustomizationPoint>, ptrdiff_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_size_t<RangeLikeCustomizationPoint>, size_t>);
        static_assert(AZStd::same_as<AZStd::ranges::range_value_t<RangeLikeCustomizationPoint>, RangeLikeCustomizationPoint>);
        static_assert(AZStd::same_as<AZStd::ranges::range_reference_t<RangeLikeCustomizationPoint>, RangeLikeCustomizationPoint&>);
        static_assert(AZStd::same_as<AZStd::ranges::range_rvalue_reference_t<RangeLikeCustomizationPoint>, RangeLikeCustomizationPoint&&>);
    }

    // Ranges Concepts Test
    TEST_F(RangesTestFixture, RangesConcepts_Compiles)
    {
        using ArrayExtentType = int[5];
        // concept - range
        static_assert(AZStd::ranges::range<AZStd::string_view>);
        static_assert(AZStd::ranges::range<ArrayExtentType>);
        static_assert(AZStd::ranges::range<AZ::IO::PathView>);
        static_assert(!AZStd::ranges::range<int>);

        // concept - sized_range
        static_assert(AZStd::ranges::sized_range<AZStd::string_view>);
        static_assert(AZStd::ranges::sized_range<ArrayExtentType>);
        // Path classes do not have a size() function so they are not a sized_range
        static_assert(!AZStd::ranges::sized_range<AZ::IO::PathView>);

        // concept - borrowed_range
        static_assert(AZStd::ranges::borrowed_range<AZStd::string_view>);
        static_assert(AZStd::ranges::borrowed_range<ArrayExtentType&>);
        static_assert(!AZStd::ranges::borrowed_range<ArrayExtentType>);

        // concept - output_range
        static_assert(AZStd::ranges::output_range<AZStd::string, char>);
        static_assert(!AZStd::ranges::output_range<AZStd::string_view, char>);

        // concept - input_range
        static_assert(AZStd::ranges::input_range<AZStd::list<int>>);
        static_assert(AZStd::ranges::input_range<AZStd::string>);
        static_assert(AZStd::ranges::input_range<AZStd::string_view>);

        // concept - forward_range
        static_assert(AZStd::ranges::forward_range<AZStd::list<int>>);
        static_assert(AZStd::ranges::forward_range<AZStd::string>);
        static_assert(AZStd::ranges::forward_range<AZStd::string_view>);

        // concept - bidirectional_range
        static_assert(AZStd::ranges::bidirectional_range<AZStd::list<int>>);
        static_assert(AZStd::ranges::bidirectional_range<AZStd::string>);
        static_assert(AZStd::ranges::bidirectional_range<AZStd::string_view>);

        // concept - random_access_range
        static_assert(!AZStd::ranges::random_access_range<AZStd::list<int>>);
        static_assert(AZStd::ranges::random_access_range<AZStd::deque<int>>);
        static_assert(AZStd::ranges::random_access_range<AZStd::string>);
        static_assert(AZStd::ranges::random_access_range<AZStd::string_view>);

        // concept - contiguous_range
        static_assert(!AZStd::ranges::contiguous_range<AZStd::deque<int>>);
        static_assert(AZStd::ranges::contiguous_range<AZStd::string>);
        static_assert(AZStd::ranges::contiguous_range<AZStd::string_view>);

        // concept - common_range
        static_assert(AZStd::ranges::common_range<ArrayExtentType>);
        static_assert(AZStd::ranges::common_range<AZStd::list<int>>);
        static_assert(AZStd::ranges::common_range<AZStd::deque<int>>);
        static_assert(AZStd::ranges::common_range<AZStd::string>);
        static_assert(AZStd::ranges::common_range<AZStd::string_view>);

        // concept - view
        static_assert(AZStd::ranges::view<AZStd::string_view>);
        static_assert(!AZStd::ranges::view<ArrayExtentType>);

        // concept - viewable_range
        static_assert(AZStd::ranges::viewable_range<AZStd::string>);
        static_assert(AZStd::ranges::viewable_range<AZStd::string_view>);
        static_assert(!AZStd::ranges::viewable_range<ArrayExtentType>);
    }

    // Ranges iterator operations
    TEST_F(RangesTestFixture, RangesAdvance_PositiveDifference_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto strIter = testString.begin();

        // difference overload
        AZStd::ranges::advance(strIter, 5);
        ASSERT_NE(testString.end(), strIter);
        EXPECT_EQ(' ', *strIter);

        // bound overload
        AZStd::ranges::advance(strIter, testString.end());
        EXPECT_EQ(testString.end(), strIter);

        // difference + bound overload
        strIter = testString.begin();
        ptrdiff_t charactersToTraverse = 20;
        EXPECT_EQ(charactersToTraverse - testString.size(), AZStd::ranges::advance(strIter, charactersToTraverse, testString.end()));
        EXPECT_EQ(testString.end(), strIter);

        strIter = testString.begin();
        charactersToTraverse = 5;
        EXPECT_EQ(0, AZStd::ranges::advance(strIter, charactersToTraverse, testString.end()));
        ASSERT_NE(testString.end(), strIter);
        EXPECT_EQ(' ', *strIter);
    }

    TEST_F(RangesTestFixture, RangesAdvance_NegativeDifference_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto strIter = testString.end();

        // difference overload
        AZStd::ranges::advance(strIter, -5);
        ASSERT_NE(testString.end(), strIter);
        EXPECT_EQ('W', *strIter);

        // difference + bound overload
        strIter = testString.end();
        ptrdiff_t charactersToTraverse = -20;
        EXPECT_EQ(charactersToTraverse + testString.size(), AZStd::ranges::advance(strIter, charactersToTraverse, testString.begin()));
        EXPECT_EQ(testString.begin(), strIter);

        strIter = testString.end();
        charactersToTraverse = -5;
        EXPECT_EQ(0, AZStd::ranges::advance(strIter, charactersToTraverse, testString.begin()));
        ASSERT_NE(testString.end(), strIter);
        ASSERT_NE(testString.begin(), strIter);
        EXPECT_EQ('W', *strIter);
    }

    TEST_F(RangesTestFixture, RangesDistance_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        EXPECT_EQ(testString.size(), AZStd::ranges::distance(testString));
        EXPECT_EQ(testString.size(), AZStd::ranges::distance(testString.begin(), testString.end()));

        AZStd::list<char> testList{ 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' };
        EXPECT_EQ(testList.size(), AZStd::ranges::distance(testList));
        EXPECT_EQ(testList.size(), AZStd::ranges::distance(testList.begin(), testList.end()));
    }

    TEST_F(RangesTestFixture, RangesNext_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto strIter = testString.begin();
        auto boundIter = testString.begin() + 5;
        // single increment
        EXPECT_EQ(testString.begin() + 1, AZStd::ranges::next(strIter));
        // increment by value
        strIter = testString.begin();
        EXPECT_EQ(testString.begin() + 5, AZStd::ranges::next(strIter, 5));
        // increment until bound
        strIter = testString.begin();
        EXPECT_EQ(testString.begin() + 5, AZStd::ranges::next(strIter, boundIter));
        // increment by value up until bound
        strIter = testString.begin();
        EXPECT_EQ(testString.begin() + 5, AZStd::ranges::next(strIter, 10, boundIter));
        strIter = testString.begin();
        EXPECT_EQ(testString.begin() + 4, AZStd::ranges::next(strIter, 4, boundIter));
    }

    TEST_F(RangesTestFixture, RangesPrev_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto strIter = testString.end();
        auto boundIter = testString.end() - 5;
        // single decrement
        EXPECT_EQ(testString.end() - 1, AZStd::ranges::prev(strIter));
        // decrement by value
        strIter = testString.end();
        EXPECT_EQ(testString.end() - 5, AZStd::ranges::prev(strIter, 5));
        // decrement by value up until bound
        strIter = testString.end();
        EXPECT_EQ(testString.end() - 5, AZStd::ranges::prev(strIter, 10, boundIter));
        strIter = testString.end();
        EXPECT_EQ(testString.end() - 4, AZStd::ranges::prev(strIter, 4, boundIter));
    }
}

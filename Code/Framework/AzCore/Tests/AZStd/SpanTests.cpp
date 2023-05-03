/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/span.h>

namespace UnitTest
{
    class SpanTestFixture
        : public LeakDetectionFixture
    {};

    // range access
    TEST_F(SpanTestFixture, IsConstructibleWithContiguousRangeLikeContainers)
    {
        constexpr AZStd::string_view testStringView{ "Foo" };
        AZStd::string testString{ "Foo" };
        AZStd::vector testVector{ 'F', 'o', 'o' };
        AZStd::fixed_vector testFixedVector{ 'F', 'o', 'o' };
        AZStd::array testStdArray{ 'F', 'o', 'o' };
        const char testCArray[]{ 'F', 'o', 'o' };

        constexpr AZStd::span stringViewSpan(testStringView);
        static_assert(stringViewSpan.data() == testStringView.data());

        AZStd::span testStringSpan(testString);
        EXPECT_EQ(testStringSpan.data(), testString.data());

        AZStd::span testVectorSpan(testVector);
        EXPECT_EQ(testVectorSpan.data(), testVector.data());

        AZStd::span testFixedVectorSpan(testFixedVector);
        EXPECT_EQ(testFixedVectorSpan.data(), testFixedVector.data());

        AZStd::span testStdArraySpan(testStdArray);
        EXPECT_EQ(testStdArraySpan.data(), testStdArray.data());

        AZStd::span testCArraySpan(testCArray);
        EXPECT_EQ(AZStd::data(testCArray), testCArraySpan.data());
    }

    TEST_F(SpanTestFixture, IsConstructibleWithContiguousIterators)
    {
        constexpr AZStd::string_view testStringView{ "Foo" };
        AZStd::string testString{ "Foo" };
        AZStd::vector testVector{ 'F', 'o', 'o' };
        AZStd::fixed_vector testFixedVector{ 'F', 'o', 'o' };
        AZStd::array testStdArray{ 'F', 'o', 'o' };
        const char testCArray[]{ 'F', 'o', 'o' };

        constexpr AZStd::span stringViewSpan(testStringView.begin(), testStringView.end());
        static_assert(stringViewSpan.data() == testStringView.data());

        AZStd::span testStringSpan(testString.begin(), testString.end());
        EXPECT_EQ(testStringSpan.data(), testString.data());

        AZStd::span testVectorSpan(testVector.begin(), testVector.end());
        EXPECT_EQ(testVectorSpan.data(), testVector.data());

        AZStd::span testFixedVectorSpan(testFixedVector.begin(), testFixedVector.end());
        EXPECT_EQ(testFixedVectorSpan.data(), testFixedVector.data());

        AZStd::span testStdArraySpan(testStdArray.begin(), testStdArray.end());
        EXPECT_EQ(testStdArraySpan.data(), testStdArray.data());

        AZStd::span testCArraySpan(AZStd::begin(testCArray), AZStd::end(testCArray));
        EXPECT_EQ(AZStd::data(testCArray), testCArraySpan.data());
    }

    TEST_F(SpanTestFixture, ObserverMethods_ReturnsCorrectValues)
    {
        AZStd::vector<int> intVector{ 4, 5, 6, 1, 7 };

        AZStd::span intSpan(intVector);

        EXPECT_FALSE(intSpan.empty());
        EXPECT_EQ(intVector.size(), intSpan.size());
        EXPECT_EQ(intSpan.size() * sizeof(int), intSpan.size_bytes());

        intSpan = {};

        EXPECT_TRUE(intSpan.empty());
        EXPECT_EQ(0, intSpan.size());
        EXPECT_EQ(0, intSpan.size_bytes());
    }

    TEST_F(SpanTestFixture, ElementAccessorMethods_Succeeds)
    {
        AZStd::vector<int> intVector{ 4, 5, 6, 1, 7 };

        AZStd::span intSpan(intVector);

        EXPECT_EQ(intVector.data(), intSpan.data());
        EXPECT_EQ(4, intSpan.front());
        EXPECT_EQ(7, intSpan.back());
        EXPECT_EQ(6, intSpan[2]);

        // Create subspan from elements 1 .. end - 1
        intSpan = intSpan.subspan(1, intSpan.size() - 2);
        EXPECT_NE(intVector.data(), intSpan.data());
        EXPECT_EQ(5, intSpan.front());
        EXPECT_EQ(1, intSpan.back());
        EXPECT_EQ(6, intSpan[1]);
    }

    TEST_F(SpanTestFixture, Supspan_Returns_Subview_Succeeds)
    {
        AZStd::vector<int> intVector{ 4, 5, 6, 1, 7 };

        AZStd::span intSpan(intVector);

        // dynamic_extent subspan with count
        auto dynamicIntSubSpan = intSpan.subspan(1, 2);
        ASSERT_EQ(2, dynamicIntSubSpan.size());
        EXPECT_EQ(5, dynamicIntSubSpan[0]);
        EXPECT_EQ(6, dynamicIntSubSpan[1]);

        // dynamic_extent subspan without count
        dynamicIntSubSpan = intSpan.subspan(1);
        ASSERT_EQ(4, dynamicIntSubSpan.size());
        EXPECT_EQ(5, dynamicIntSubSpan[0]);
        EXPECT_EQ(6, dynamicIntSubSpan[1]);
        EXPECT_EQ(1, dynamicIntSubSpan[2]);
        EXPECT_EQ(7, dynamicIntSubSpan[3]);

        // template subspan with count
        auto templateIntSubSpan1 = intSpan.subspan<1, 3>();
        static_assert(decltype(templateIntSubSpan1)::extent == 3);
        ASSERT_EQ(3, templateIntSubSpan1.size());
        EXPECT_EQ(5, templateIntSubSpan1[0]);
        EXPECT_EQ(6, templateIntSubSpan1[1]);
        EXPECT_EQ(1, templateIntSubSpan1[2]);

        // template subspan without count
        auto templateIntSubSpan2 = intSpan.subspan<1>();
        static_assert(decltype(templateIntSubSpan2)::extent == AZStd::dynamic_extent);
        ASSERT_EQ(4, templateIntSubSpan2.size());
        EXPECT_EQ(5, templateIntSubSpan2[0]);
        EXPECT_EQ(6, templateIntSubSpan2[1]);
        EXPECT_EQ(1, templateIntSubSpan2[2]);
        EXPECT_EQ(7, templateIntSubSpan2[3]);

        // get subspan of fixed extent span without count
        auto subSpanOfSubSpan = templateIntSubSpan1.subspan<1>();
        static_assert(decltype(subSpanOfSubSpan)::extent == 2);
        ASSERT_EQ(2, subSpanOfSubSpan.size());
        EXPECT_EQ(6, subSpanOfSubSpan[0]);
        EXPECT_EQ(1, subSpanOfSubSpan[1]);
    }

    TEST_F(SpanTestFixture, FirstMethod_Returns_FirstCountElementsOfSpan)
    {
        constexpr size_t vectorElementCount = 5;
        AZStd::vector<int> intVector{ 4, 5, 6, 1, 7 };

        AZStd::span intSpan(intVector);

        {
            // No templated first function
            auto prefixSpan = intSpan.first(3);
            ASSERT_EQ(3, prefixSpan.size());
            EXPECT_EQ(4, prefixSpan[0]);
            EXPECT_EQ(5, prefixSpan[1]);
            EXPECT_EQ(6, prefixSpan[2]);

            auto prefixSpanRedux = prefixSpan.first(1);
            ASSERT_EQ(1, prefixSpanRedux.size());
            EXPECT_EQ(4, prefixSpanRedux[0]);

            // Test failure of preconditions by requesting more
            // elements thant stored in the span
            AZ_TEST_START_TRACE_SUPPRESSION;
            intSpan.first(intSpan.size() + 1);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        {
            // templated first function
            auto prefixSpan = intSpan.first<3>();
            static_assert(decltype(prefixSpan)::extent == 3);
            ASSERT_EQ(3, prefixSpan.size());
            EXPECT_EQ(4, prefixSpan[0]);
            EXPECT_EQ(5, prefixSpan[1]);
            EXPECT_EQ(6, prefixSpan[2]);

            auto prefixSpanRedux = prefixSpan.first<1>();
            ASSERT_EQ(1, prefixSpanRedux.size());
            EXPECT_EQ(4, prefixSpanRedux[0]);

            // Test failure of preconditions by requesting more
            // elements thant stored in the span
            AZ_TEST_START_TRACE_SUPPRESSION;
            intSpan.first<vectorElementCount + 1>();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }
    }

    TEST_F(SpanTestFixture, LastCountElementsOfSpan)
    {
        constexpr size_t vectorElementCount = 5;
        AZStd::vector<int> intVector{ 4, 5, 6, 1, 7 };

        AZStd::span intSpan(intVector);

        {
            // No templated last function
            auto suffixSpan = intSpan.last(3);
            ASSERT_EQ(3, suffixSpan.size());
            EXPECT_EQ(6, suffixSpan[0]);
            EXPECT_EQ(1, suffixSpan[1]);
            EXPECT_EQ(7, suffixSpan[2]);

            auto suffixSpanRedux = suffixSpan.last(1);
            ASSERT_EQ(1, suffixSpanRedux.size());
            EXPECT_EQ(7, suffixSpanRedux[0]);

            // Test failure of preconditions by requesting more
            // elements thant stored in the span
            AZ_TEST_START_TRACE_SUPPRESSION;
            intSpan.last(intSpan.size() + 1);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        {
            // templated last function
            auto suffixSpan = intSpan.last<3>();
            static_assert(decltype(suffixSpan)::extent == 3);
            ASSERT_EQ(3, suffixSpan.size());
            EXPECT_EQ(6, suffixSpan[0]);
            EXPECT_EQ(1, suffixSpan[1]);
            EXPECT_EQ(7, suffixSpan[2]);

            auto suffixSpanRedux = suffixSpan.last<1>();
            ASSERT_EQ(1, suffixSpanRedux.size());
            EXPECT_EQ(7, suffixSpanRedux[0]);

            // Test failure of preconditions by requesting more
            // elements thant stored in the span
            AZ_TEST_START_TRACE_SUPPRESSION;
            intSpan.last<vectorElementCount + 1>();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }
    }

    TEST_F(SpanTestFixture, CanInitializeFixedArrayToDynamicExtentSpan)
    {
        constexpr size_t arrayElementCount = 5;
        static constexpr AZStd::array<int, arrayElementCount> intArray{ 4, 5, 6, 1, 7 };

        constexpr AZStd::span<const int, AZStd::dynamic_extent> arraySpan(intArray);
        static_assert(intArray.data() == arraySpan.data());

        constexpr AZStd::span<const int, arrayElementCount> arraySpanFixedExtent(intArray);
        static_assert(intArray.data() == arraySpanFixedExtent.data());
    }
}

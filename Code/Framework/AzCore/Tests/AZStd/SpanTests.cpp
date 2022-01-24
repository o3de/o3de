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
        : public ScopedAllocatorSetupFixture
    {};

    // range access
    TEST_F(SpanTestFixture, IsConstructibleWithContigousRangeLikeContainers)
    {
        constexpr AZStd::string_view testStringView{ "Foo" };
        AZStd::string testString{ "Foo" };
        AZStd::vector testVector{ 'F', 'o', 'o' };
        AZStd::fixed_vector testFixedVector{ 'F', 'o', 'o' };
        AZStd::array testStdArray{ 'F', 'o', 'o' };
        const char testCArray[]{'F', 'o', 'o'};

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

    TEST_F(SpanTestFixture, IsConstructibleWithContigousIterators)
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
}

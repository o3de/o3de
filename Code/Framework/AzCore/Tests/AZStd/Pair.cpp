/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/compressed_pair.h>
#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    template<typename TestConfig>
    class CompressedPairTest
        : public LeakDetectionFixture
    {
    };

    template<typename TestConfig>
    class CompressedPairSizeTest
        : public CompressedPairTest<TestConfig>
    {};

    namespace CompressedPairInternal
    {
        struct EmptyStruct
        {};

        struct EmptyStructNonDerived
        {};

        struct FinalEmptyStruct final
        {};

        struct DerivedFromEmptyStruct
            : EmptyStruct
        {

        };

        struct DerivedWithDataFromEmptyStruct
            : EmptyStruct
        {
            uint32_t m_value{};
        };
    }

    template<typename T1, typename T2, size_t MaxExpectedSize>
    struct CompressedPairTestConfig
    {
        using first_type = T1;
        using second_type = T2;
        static constexpr size_t max_expected_size = MaxExpectedSize;
    };

    using CompressedPairTestConfigs = ::testing::Types<
        CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, CompressedPairInternal::FinalEmptyStruct, 1>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, int32_t, 4>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, CompressedPairInternal::EmptyStructNonDerived, 1>
        , CompressedPairTestConfig<int32_t, CompressedPairInternal::EmptyStruct, 4>
        , CompressedPairTestConfig<int32_t, int32_t, 8>
    >;
    TYPED_TEST_CASE(CompressedPairTest, CompressedPairTestConfigs);

    using CompressedPairSizeTestConfigs = ::testing::Types<
        CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, CompressedPairInternal::FinalEmptyStruct, 1>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, CompressedPairInternal::EmptyStructNonDerived, 1>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStruct, int32_t, 4>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStructNonDerived, CompressedPairInternal::FinalEmptyStruct, 1>
        , CompressedPairTestConfig<CompressedPairInternal::EmptyStructNonDerived, CompressedPairInternal::DerivedFromEmptyStruct, 1>
        , CompressedPairTestConfig<CompressedPairInternal::FinalEmptyStruct, int32_t, 8>
        , CompressedPairTestConfig<CompressedPairInternal::FinalEmptyStruct, CompressedPairInternal::FinalEmptyStruct, 2>
        , CompressedPairTestConfig<CompressedPairInternal::FinalEmptyStruct, CompressedPairInternal::DerivedWithDataFromEmptyStruct, 8>
        , CompressedPairTestConfig<CompressedPairInternal::DerivedWithDataFromEmptyStruct, CompressedPairInternal::EmptyStructNonDerived, 4>
        , CompressedPairTestConfig<CompressedPairInternal::DerivedWithDataFromEmptyStruct, CompressedPairInternal::DerivedWithDataFromEmptyStruct, 8>
        , CompressedPairTestConfig<int32_t, int32_t, 8>
    >;
    TYPED_TEST_CASE(CompressedPairSizeTest, CompressedPairSizeTestConfigs);

    TYPED_TEST(CompressedPairTest, CompressedPairDefaultConstructorSucceeds)
    {
        AZStd::compressed_pair<typename TypeParam::first_type, typename TypeParam::second_type> testPair;
        (void)testPair;
    }

    TYPED_TEST(CompressedPairTest, CompressedPairFirstElementConstructorSucceeds)
    {
        AZStd::compressed_pair<typename TypeParam::first_type, typename TypeParam::second_type> testPair(typename TypeParam::first_type{});
        (void)testPair;
    }

    TYPED_TEST(CompressedPairTest, CompressedPairSecondElementConstructorSucceeds)
    {
        AZStd::compressed_pair<typename TypeParam::first_type, typename TypeParam::second_type> testPair(AZStd::skip_element_tag{}, typename TypeParam::second_type{});
        (void)testPair;
    }

    TYPED_TEST(CompressedPairTest, CompressedPairPiecewiseElementConstructorSucceeds)
    {
        AZStd::compressed_pair<typename TypeParam::first_type, typename TypeParam::second_type> testPair(AZStd::piecewise_construct_t{}, std::tuple<>{}, std::tuple<>{});
        (void)testPair;
    }

    TYPED_TEST(CompressedPairSizeTest, CompressedPairUsesEmptyBaseOptimizationForClassSize)
    {
        static_assert(sizeof(TypeParam) <= TypeParam::max_expected_size, "Compressed Pair is not expected size");
    }

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

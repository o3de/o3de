/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/variant.h>
#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class VariantTest
        : public LeakDetectionFixture
    {
    };

    template<typename TestConfig>
    class VariantSizeTest
        : public LeakDetectionFixture
    {
    };

    template<typename VariantType, size_t ExpectedSize>
    struct VariantSizeTestConfig
    {
        using type = VariantType;
        static constexpr size_t expected_size = ExpectedSize;
    };
    using VariantSizeTestConfigs = ::testing::Types<
        VariantSizeTestConfig<AZStd::variant<AZStd::monostate>, 1>
        , VariantSizeTestConfig<AZStd::variant<void*, const void*>, 2>
        , VariantSizeTestConfig<AZStd::variant<uint64_t, AZStd::string, uint64_t, double>, 4>
    >;
    TYPED_TEST_CASE(VariantSizeTest, VariantSizeTestConfigs);

    template<typename TestConfig>
    class VariantAlternativeTest
        : public LeakDetectionFixture
    {
    };

    template<typename VariantType, size_t Index, typename AlternativeType>
    struct VariantAlternativeTestConfig
    {
        using type = VariantType;
        using expected_type = AlternativeType;
        static constexpr size_t expected_index = Index;
    };
    using Variant6AltType = AZStd::variant<AZStd::monostate, void*, const void*, double, int32_t, uintptr_t>;
    using VariantAlternativeTestConfigs = ::testing::Types<
        VariantAlternativeTestConfig<Variant6AltType, 0, AZStd::monostate>
        , VariantAlternativeTestConfig<Variant6AltType, 1, void*>
        , VariantAlternativeTestConfig<Variant6AltType, 2, const void*>
        , VariantAlternativeTestConfig<Variant6AltType, 3, double>
        , VariantAlternativeTestConfig<Variant6AltType, 4, int32_t>
        , VariantAlternativeTestConfig<Variant6AltType, 5, uintptr_t>
    >;
    TYPED_TEST_CASE(VariantAlternativeTest, VariantAlternativeTestConfigs);

    namespace VariantTestInternal
    {
        struct TrivialAlt
        {};

        struct NonDefaultConstructible
        {
            constexpr NonDefaultConstructible(bool)
            {}
        };
        struct MoveConstructorOnly
        {
            MoveConstructorOnly() = default;
            MoveConstructorOnly(const MoveConstructorOnly&) = delete;
            MoveConstructorOnly(MoveConstructorOnly&& other)
            {
                memcpy(m_alignedBuffer, other.m_alignedBuffer, sizeof(m_alignedBuffer));
                memset(other.m_alignedBuffer, 0, sizeof(other.m_alignedBuffer));

            }
            alignas(16) uint8_t m_alignedBuffer[32];
        };
        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&& other)
            {
                m_value = other.m_value;
                other.m_value = {};
                m_moveConstructed = true;
            }

            MoveOnly& operator=(MoveOnly&& rhs)
            {
                m_value = rhs.m_value;
                rhs.m_value = {};
                ++m_moveAssignmentCounter;
                return *this;
            }

            int32_t m_value{};
            int32_t m_moveAssignmentCounter{};
            bool m_moveConstructed{};
        };
        struct CopyConstructorOnly
        {
            CopyConstructorOnly() = default;
            CopyConstructorOnly(const CopyConstructorOnly& other)
            {
                memcpy(m_alignedBuffer, other.m_alignedBuffer, sizeof(m_alignedBuffer));
            }

            CopyConstructorOnly(CopyConstructorOnly&& other) = delete;

            alignas(16) uint8_t m_alignedBuffer[32];
        };

        struct ConstexprVisitorHelper
        {
            template <typename... Args>
            constexpr int64_t operator()(int64_t numAlt, Args&&...) const
            {
                return numAlt;
            }
        };

        struct VariadicVariantSizeof
        {
            template <typename... Args>
            constexpr size_t operator()(Args&&...) const
            {
                return sizeof...(Args);
            }
        };

        enum class MemberFuncQualifiers : uint32_t
        {
            const_ = 1,
            lvalue_ref = 2,
            rvalue_ref = 4,
            const_lvalue_ref = const_ | lvalue_ref,
            const_rvalue_ref = const_ | rvalue_ref,
        };

        struct MemberFuncQualifierCallable
        {
            template <typename... Args>
            MemberFuncQualifiers operator()(Args&&...) &
            {
                return MemberFuncQualifiers::lvalue_ref;
            }

            template <typename... Args>
            MemberFuncQualifiers operator()(Args&&...) const &
            {
                return MemberFuncQualifiers::const_lvalue_ref;
            }

            template <typename... Args>
            MemberFuncQualifiers operator()(Args&&...) &&
            {
                return MemberFuncQualifiers::rvalue_ref;
            }

            template <typename... Args>
            MemberFuncQualifiers operator()(Args&&...) const &&
            {
                return MemberFuncQualifiers::const_rvalue_ref;
            }
        };

        struct ParameterQualifierCallable
        {
            template <typename... Args>
            MemberFuncQualifiers operator()(Args&&...)
            {
                constexpr bool lvalueArg = AZStd::conjunction<AZStd::is_lvalue_reference<Args>...>::value;
                constexpr bool constArg = AZStd::conjunction<AZStd::is_const<AZStd::remove_reference_t<Args>>...>::value;

                AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
                if (lvalueArg && !constArg)
                {
                    return MemberFuncQualifiers::lvalue_ref;
                }
                else if (lvalueArg && constArg)
                {
                    return MemberFuncQualifiers::const_lvalue_ref;
                }
                else if (!lvalueArg && !constArg)
                {
                    return MemberFuncQualifiers::rvalue_ref;
                }
                else if (!lvalueArg && constArg)
                {
                    return MemberFuncQualifiers::const_rvalue_ref;
                }
                AZ_POP_DISABLE_WARNING

                return MemberFuncQualifiers{};
            }
        };

        template <typename Sequence>
        struct make_heterogeneous_variant;

        template <size_t... Indices>
        struct make_heterogeneous_variant<AZStd::index_sequence<Indices...>>
        {
            template <size_t Index>
            using alternative_t = uint64_t;
            using type = AZStd::variant<alternative_t<Indices>...>;
        };

        template <size_t NumAlternatives>
        using make_heterogeneous_variant_t = typename make_heterogeneous_variant<AZStd::make_index_sequence<NumAlternatives>>::type;
    }

    // variant_size test
    TYPED_TEST(VariantSizeTest, variant_sizeReturnsCorrectSizeWithConstVolatileQualifiers)
    {
        static_assert(TypeParam::expected_size == AZStd::variant_size<typename TypeParam::type>::value, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size<const typename TypeParam::type>::value, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size<volatile typename TypeParam::type>::value, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size<const volatile typename TypeParam::type>::value, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size_v<typename TypeParam::type>, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size_v<const typename TypeParam::type>, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size_v<volatile typename TypeParam::type>, "Variant size is not equal to expected result");
        static_assert(TypeParam::expected_size == AZStd::variant_size_v<const volatile typename TypeParam::type>, "Variant size is not equal to expected result");
    }

    TEST_F(VariantTest, variant_sizeReturnsNumberOfAlternatives)
    {
        using TestVariant1 = AZStd::variant<int, float, bool, AZStd::string_view>;
        static_assert(AZStd::variant_size_v<TestVariant1> == 4, "Variant size should be equal to the number of alternatives");
        using TestVariant2 = AZStd::variant<int, float, bool, AZStd::string, float>;
        static_assert(AZStd::variant_size_v<TestVariant2> == 5, "Variant size should be equal to the number of alternatives");
    }

    // variant_alternative test
    TYPED_TEST(VariantAlternativeTest, variant_alternativeReturnsCorrectTypeForIndex)
    {
        static_assert(AZStd::is_same_v<typename TypeParam::expected_type, typename AZStd::variant_alternative<TypeParam::expected_index, typename TypeParam::type>::type>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<const typename TypeParam::expected_type, typename AZStd::variant_alternative<TypeParam::expected_index, const typename TypeParam::type>::type>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<volatile typename TypeParam::expected_type, typename AZStd::variant_alternative<TypeParam::expected_index, volatile typename TypeParam::type>::type>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<const volatile typename TypeParam::expected_type, typename AZStd::variant_alternative<TypeParam::expected_index, const volatile typename TypeParam::type>::type>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<typename TypeParam::expected_type, AZStd::variant_alternative_t<TypeParam::expected_index, typename TypeParam::type>>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<const typename TypeParam::expected_type, AZStd::variant_alternative_t<TypeParam::expected_index, const typename TypeParam::type>>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<volatile typename TypeParam::expected_type, AZStd::variant_alternative_t<TypeParam::expected_index, volatile typename TypeParam::type>>, "Variant size is not equal to expected result");
        static_assert(AZStd::is_same_v<const volatile typename TypeParam::expected_type, AZStd::variant_alternative_t<TypeParam::expected_index, const volatile typename TypeParam::type>>, "Variant size is not equal to expected result");
    }

    TEST_F(VariantTest, sizeof_VariantIsGreaterThanLargestElementSize)
    {
        // Largest Element is string_view
        using TestVariant1 = AZStd::variant<int, float, bool, AZStd::string_view>;
        
        static_assert(sizeof(AZStd::string_view) < sizeof(TestVariant1), "using the sizeof operator on a std::variant should return a size larger than all the alternatives");
        // Largest Element is MoveOnly
        using TestVariant2 = AZStd::variant<double, AZStd::string_view, VariantTestInternal::MoveConstructorOnly, bool, float>;
        static_assert(sizeof(VariantTestInternal::MoveConstructorOnly) < sizeof(TestVariant2), "using the sizeof operator on a std::variant should return a size larger than all the alternatives");
    }

    TEST_F(VariantTest, sizeof_VariantIsLessThanCombinedAlternativeSize)
    {
        // Largest Element is string_view
        using TestVariant1 = AZStd::variant<int, float, bool, AZStd::string_view, AZStd::string_view>;
        // For a variant with less than 255 alternatives the index type is an uint8_t
        constexpr size_t variantAlignment = alignof(TestVariant1);
        constexpr size_t maxExpectedVariantByteSize = (sizeof(AZStd::string_view) + sizeof(uint8_t) + variantAlignment - 1) & ~(variantAlignment - 1);
        constexpr size_t testVariantByteSize = sizeof(TestVariant1);
        static_assert(testVariantByteSize <= maxExpectedVariantByteSize, "using the sizeof operator on a std::variant should return a size smaller"
            " than the sizeof(largest type) + padding + sizeof(index type)");
    }

    TEST_F(VariantTest, DifferentPermutationsOfVariantHasSameSize)
    {
        constexpr size_t alignedStorageSize = 144;
        using TestAlignedStorage = AZStd::aligned_storage_t<alignedStorageSize, 16>;
        // Largest Element is string_view
        using TestVariant1 = AZStd::variant<int, float, bool, TestAlignedStorage>;
        using TestVariant2 = AZStd::variant<TestAlignedStorage, float, bool, int>;
        static_assert(sizeof(TestVariant1) == sizeof(TestVariant2), "with different permutations variants of same types should be the same size");
        using UnorderedVariant3 = AZStd::variant<AZStd::unordered_map<AZStd::string, AZStd::string>, TestAlignedStorage>;
        static_assert(sizeof(TestVariant1) == sizeof(UnorderedVariant3), "with different permutations variants of same types should be the same size");
    }

    TEST_F(VariantTest, VariantIndexTypeIsLessSizeIfLessThan255Alternatives)
    {
        constexpr size_t indexSizeWithOneAlternatives = sizeof(AZStd::variant_index_t<1>);
        constexpr size_t indexSizeWithOneByteMaxAlternatives = sizeof(AZStd::variant_index_t<std::numeric_limits<uint8_t>::max() - 1>);
        constexpr size_t indexSizeWithTwoByteMinAlternatives = sizeof(AZStd::variant_index_t<std::numeric_limits<uint8_t>::max()>);
        static_assert(indexSizeWithOneAlternatives == indexSizeWithOneByteMaxAlternatives, "the index type for a variant with 1 alternative should be the same size for a variant with 254 alternatives");
        static_assert(indexSizeWithOneByteMaxAlternatives < indexSizeWithTwoByteMinAlternatives, "the index type for a variant with 254 alternatives should be smaller for a variant with 255 alternatives");
    }

    namespace Constructors
    {
        TEST_F(VariantTest, DefaultConstructorVariantSetsIndexZero)
        {
            constexpr AZStd::variant<AZStd::monostate> monostateVariant;
            static_assert(monostateVariant.index() == 0U, "Default Constructed variant should be constructed to first alternative");
            EXPECT_EQ(0U, monostateVariant.index());
        }
        TEST_F(VariantTest, DefaultConstructorSucceedsWhenOnlyFirstAlternativeIsDefaultConstructible)
        {
            AZStd::variant<AZStd::string_view, VariantTestInternal::NonDefaultConstructible> testVariant;
            EXPECT_EQ(0U, testVariant.index());
            EXPECT_TRUE(AZStd::get<0>(testVariant).empty());
        }

        TEST_F(VariantTest, ConvertingConstructorSucceeds)
        {
            const AZStd::variant<AZStd::string> stringVariant{ AZStd::string("Test String") };
            const AZStd::variant<AZStd::string> convertToStringVariant{ "Test String" };
            EXPECT_EQ(stringVariant, convertToStringVariant);
        }

        TEST_F(VariantTest, CopyConstructorSucceeds)
        {
            AZStd::variant<uint32_t, AZStd::string> numberStringUnion{ 5U };
            AZStd::variant<uint32_t, AZStd::string> copiedUnion(numberStringUnion);
            EXPECT_EQ(copiedUnion, numberStringUnion);

            const AZStd::variant<AZStd::string, int32_t> testVariant3{ AZStd::in_place_index_t<1>{}, 6 };
            const AZStd::variant<AZStd::string, int32_t > copyVariant(testVariant3);
            EXPECT_EQ(testVariant3, copyVariant);
            ASSERT_EQ(1, copyVariant.index());
            EXPECT_EQ(6, AZStd::get<1>(copyVariant));
        }

        TEST_F(VariantTest, MoveConstructorSucceeds)
        {
            AZStd::variant<AZStd::string> sourceStringUnion("Hello World");
            AZStd::variant<AZStd::string> moveUnion(AZStd::move(sourceStringUnion));
            EXPECT_NE(moveUnion, sourceStringUnion);
            // Moved from Variant should have default value
            EXPECT_TRUE(AZStd::get<0>(sourceStringUnion).empty());

            AZStd::variant<AZStd::string, AZStd::string_view> testVariant3{ AZStd::in_place_index_t<1>{}, "Hello" };
            AZStd::variant<AZStd::string, AZStd::string_view > moveVariant(AZStd::move(testVariant3));
            EXPECT_NE(testVariant3, moveVariant);
            ASSERT_EQ(1, moveVariant.index());
            EXPECT_EQ("Hello", AZStd::get<1>(moveVariant));
            // Moved from Variant should have default value
            ASSERT_EQ(1, moveVariant.index());
            EXPECT_TRUE(AZStd::get<1>(testVariant3).empty());
        }

        TEST_F(VariantTest, InPlaceTypeConstructorSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> inplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 108);
            EXPECT_EQ(1, inplaceIndexVariant.index());
            auto& int32Alt = AZStd::get<1>(inplaceIndexVariant);
            EXPECT_EQ(108, int32Alt);
        }

        TEST_F(VariantTest, InPlaceTypeConstructorWithInitializerListSucceeds)
        {
            AZStd::variant<int32_t, double, AZStd::vector<int32_t>, bool> inplaceIndexVariant(AZStd::in_place_type_t<AZStd::vector<int32_t>>{}, { 1, 2, 3 }, AZStd::allocator("Variant in_place_type allocator "));
            EXPECT_EQ(2, inplaceIndexVariant.index());
            auto& vectorAlt = AZStd::get<2>(inplaceIndexVariant);
            ASSERT_EQ(3, vectorAlt.size());
            EXPECT_EQ(1, vectorAlt[0]);
            EXPECT_EQ(2, vectorAlt[1]);
            EXPECT_EQ(3, vectorAlt[2]);
        }

        TEST_F(VariantTest, InPlaceIndexConstructorSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> inplaceIndexVariant(AZStd::in_place_index_t<2>{}, 55, true);
            EXPECT_EQ(2, inplaceIndexVariant.index());
            auto& tupleAlt= AZStd::get<2>(inplaceIndexVariant);
            EXPECT_EQ(55, AZStd::get<0>(tupleAlt));
            EXPECT_EQ(true, AZStd::get<1>(tupleAlt));
        }

        TEST_F(VariantTest, InPlaceIndexConstructorWithInitializerListSucceeds)
        {
            AZStd::variant<float, AZStd::vector<int32_t>> inplaceIndexVariant(AZStd::in_place_index_t<1>{}, { 1, 2, 3 }, AZStd::allocator("Variant in_place_index allocator "));
            EXPECT_EQ(1, inplaceIndexVariant.index());
            auto& vectorAlt = AZStd::get<1>(inplaceIndexVariant);
            ASSERT_EQ(3, vectorAlt.size());
            EXPECT_EQ(1, vectorAlt[0]);
            EXPECT_EQ(2, vectorAlt[1]);
            EXPECT_EQ(3, vectorAlt[2]);
        }
    }

    namespace Emplace
    {
        // emplace<Type>
        TEST_F(VariantTest, TypeEmplaceUpdateCurrentAlternativeSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<1>(emplaceIndexVariant));
            // Update current alternative
            int32_t& updatedInt = emplaceIndexVariant.emplace<int32_t>(252);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(252, updatedInt);
        }

        TEST_F(VariantTest, TypeEmplaceUpdateDifferentAlternativeSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<1>(emplaceIndexVariant));
            // Update different alternative
            auto& updatedTuple = emplaceIndexVariant.emplace<AZStd::tuple<int32_t, bool>>(410, true);
            EXPECT_EQ(2, emplaceIndexVariant.index());

            EXPECT_EQ(410, AZStd::get<0>(updatedTuple));
            EXPECT_TRUE(AZStd::get<1>(updatedTuple));
        }

        TEST_F(VariantTest, TypeEmplaceWithInitializerListUpdateCurrentAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double, AZStd::vector<int32_t>, bool> emplaceIndexVariant(AZStd::in_place_type_t<AZStd::vector<int32_t>>{});
            EXPECT_EQ(2, emplaceIndexVariant.index());
            EXPECT_TRUE(AZStd::get<2>(emplaceIndexVariant).empty());
            // Update current alternative
            auto& updatedVector= emplaceIndexVariant.emplace<AZStd::vector<int32_t>>({ 2, 4, 6 }, AZStd::allocator("Variant emplace allocator"));
            EXPECT_EQ(2, emplaceIndexVariant.index());

            EXPECT_EQ(3, updatedVector.size());
            EXPECT_EQ(2, updatedVector[0]);
            EXPECT_EQ(4, updatedVector[1]);
            EXPECT_EQ(6, updatedVector[2]);
        }

        TEST_F(VariantTest, TypeEmplaceWithInitializerListUpdateDifferentAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double, AZStd::vector<int32_t>, bool> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(0, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<0>(emplaceIndexVariant));
            // Update different alternative
            auto& updatedVector = emplaceIndexVariant.emplace<AZStd::vector<int32_t>>({ 1, 2, 3 }, AZStd::allocator("Variant emplace allocator"));
            EXPECT_EQ(2, emplaceIndexVariant.index());

            ASSERT_EQ(3, updatedVector.size());
            EXPECT_EQ(1, updatedVector[0]);
            EXPECT_EQ(2, updatedVector[1]);
            EXPECT_EQ(3, updatedVector[2]);
        }

        // emplace<Index>
        TEST_F(VariantTest, IndexEmplaceUpdateCurrentAlternativeSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<1>(emplaceIndexVariant));
            // Update current alternative
            int32_t& updatedInt = emplaceIndexVariant.emplace<1>(252);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(252, updatedInt);
        }

        TEST_F(VariantTest, IndexEmplaceUpdateDifferentAlternativeSucceeds)
        {
            AZStd::variant<bool, int32_t, AZStd::tuple<int32_t, bool>> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(1, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<1>(emplaceIndexVariant));
            // Update different alternative
            auto& updatedTuple = emplaceIndexVariant.emplace<2>(410, true);
            EXPECT_EQ(2, emplaceIndexVariant.index());

            EXPECT_EQ(410, AZStd::get<0>(updatedTuple));
            EXPECT_TRUE(AZStd::get<1>(updatedTuple));
        }

        TEST_F(VariantTest, IndexEmplaceWithInitializerListUpdateCurrentAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double, AZStd::vector<int32_t>, bool> emplaceIndexVariant(AZStd::in_place_type_t<AZStd::vector<int32_t>>{}, { 336 });
            EXPECT_EQ(2, emplaceIndexVariant.index());
            EXPECT_EQ(1, AZStd::get<2>(emplaceIndexVariant).size());
            EXPECT_EQ(336, AZStd::get<2>(emplaceIndexVariant).at(0));
            // Update current alternative
            auto& updatedVector = emplaceIndexVariant.emplace<2>({ 17, -54 });
            EXPECT_EQ(2, emplaceIndexVariant.index());
            
            EXPECT_EQ(2, updatedVector.size());
            EXPECT_EQ(17, updatedVector[0]);
            EXPECT_EQ(-54, updatedVector[1]);
        }

        TEST_F(VariantTest, IndexEmplaceWithInitializerListUpdateDifferentAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double, AZStd::vector<int32_t>, bool> emplaceIndexVariant(AZStd::in_place_type_t<int32_t>{}, 110);
            EXPECT_EQ(0, emplaceIndexVariant.index());
            EXPECT_EQ(110, AZStd::get<0>(emplaceIndexVariant));
            // Update different alternative
            auto& updatedVector = emplaceIndexVariant.emplace<2>({ 1, 2, 3 }, AZStd::allocator("Variant emplace allocator"));
            EXPECT_EQ(2, emplaceIndexVariant.index());

            ASSERT_EQ(3, updatedVector.size());
            EXPECT_EQ(1, updatedVector[0]);
            EXPECT_EQ(2, updatedVector[1]);
            EXPECT_EQ(3, updatedVector[2]);
        }
    }

    namespace GetAlternative
    {
        //get_if<Index>
        TEST_F(VariantTest, HoldsAlternativeSucceeds)
        {
            AZStd::variant<double, int32_t> testVariant(AZStd::in_place_type_t<int32_t>{}, 17);
            EXPECT_FALSE(AZStd::holds_alternative<double>(testVariant));
            EXPECT_TRUE(AZStd::holds_alternative<int32_t>(testVariant));

            testVariant = 8.0;
            EXPECT_TRUE(AZStd::holds_alternative<double>(testVariant));
            EXPECT_FALSE(AZStd::holds_alternative<int32_t>(testVariant));
        }

        TEST_F(VariantTest, IndexGetIfWithConstVariantSingleAlternativeSucceeds)
        {
            const AZStd::variant<int32_t> testVariant(17);
            const int32_t* testAlt = AZStd::get_if<0>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_EQ(17, *testAlt);
        }

        TEST_F(VariantTest, IndexGetIfWithConstVariantMultipleAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, double> testVariant(32.0);
            const double* testAlt = AZStd::get_if<1>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_DOUBLE_EQ(32.0, *testAlt);
        }


        TEST_F(VariantTest, IndexGetIfWithConstVariantMultipleAlternativeFails)
        {
            const AZStd::variant<int32_t, double> testVariant(32.0);
            const int32_t* testAlt = AZStd::get_if<0>(&testVariant);
            EXPECT_EQ(nullptr, testAlt);
        }

        TEST_F(VariantTest, IndexGetIfWithVariantSingleAlternativeSucceeds)
        {
            AZStd::variant<int32_t> testVariant(17);
            int32_t* testAlt = AZStd::get_if<0>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_EQ(17, *testAlt);
        }

        TEST_F(VariantTest, IndexGetIfWithVariantMultipleAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double> testVariant(32.0);
            double* testAlt = AZStd::get_if<1>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_DOUBLE_EQ(32.0, *testAlt);
        }

        // get_if<Type>
        TEST_F(VariantTest, IndexGetIfWithVariantMultipleAlternativeFails)
        {
            AZStd::variant<int32_t, double> testVariant(32.0);
            int32_t* testAlt = AZStd::get_if<0>(&testVariant);
            EXPECT_EQ(nullptr, testAlt);
        }

        TEST_F(VariantTest, TypeGetIfWithConstVariantSingleAlternativeSucceeds)
        {
            const AZStd::variant<int32_t> testVariant(17);
            const int32_t* testAlt = AZStd::get_if<int32_t>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_EQ(17, *testAlt);
        }

        TEST_F(VariantTest, TypeGetIfWithConstVariantMultipleAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, double> testVariant(32.0);
            const auto testAlt = AZStd::get_if<double>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_DOUBLE_EQ(32.0, *testAlt);
        }


        TEST_F(VariantTest, TypeGetIfWithConstVariantMultipleAlternativeFails)
        {
            const AZStd::variant<int32_t, double> testVariant(32.0);
            const auto testAlt = AZStd::get_if<int32_t>(&testVariant);
            EXPECT_EQ(nullptr, testAlt);
        }

        TEST_F(VariantTest, TypeGetIfWithVariantSingleAlternativeSucceeds)
        {
            AZStd::variant<int32_t> testVariant(17);
            auto testAlt = AZStd::get_if<int32_t>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_EQ(17, *testAlt);
        }

        TEST_F(VariantTest, TypeGetIfWithVariantMultipleAlternativeSucceeds)
        {
            AZStd::variant<int32_t, double> testVariant(32.0);
            auto testAlt = AZStd::get_if<double>(&testVariant);
            ASSERT_NE(nullptr, testAlt);
            EXPECT_DOUBLE_EQ(32.0, *testAlt);
        }

        TEST_F(VariantTest, TypeGetIfWithVariantMultipleAlternativeFails)
        {
            AZStd::variant<int32_t, double> testVariant(32.0);
            auto testAlt = AZStd::get_if<int32_t>(&testVariant);
            EXPECT_EQ(nullptr, testAlt);
        }

        // get<Index>
        TEST_F(VariantTest, IndexGetLValueVariantNonConstAlternativeSucceeds)
        {
            AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            EXPECT_EQ(6, AZStd::get<0>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<0>(testVariant)), int32_t&>::value,
                "AZStd::get should return lvalue reference to non-const alternative for non-const variant");
        }

        TEST_F(VariantTest, IndexGetLValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            EXPECT_EQ(23LL, AZStd::get<1>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<1>(testVariant)), const int64_t&>::value,
                "AZStd::get should return const lvalue reference to const alternativefor non-const variant");
        }

        TEST_F(VariantTest, IndexGetConstLValueVariantNonConstAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            EXPECT_EQ(6, AZStd::get<0>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<0>(testVariant)), const int32_t&>::value,
                "AZStd::get should return const lvalue reference to non-const alternative for const variant");
        }

        TEST_F(VariantTest, IndexGetConstLValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            const AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            EXPECT_EQ(23LL, AZStd::get<1>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<1>(testVariant)), const int64_t&>::value,
                "AZStd::get should return const lvalue reference to const alternative for const variant");
        }

        TEST_F(VariantTest, IndexGetRValueVariantNonConstAlternativeSucceeds)
        {
            AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            static_assert(AZStd::is_same<decltype(AZStd::get<0>(AZStd::move(testVariant))), int32_t&&>::value,
                "AZStd::get should return rvalue reference to non-const alternative for non-const variant");
            EXPECT_EQ(6, AZStd::get<0>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, IndexGetRValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            
            static_assert(AZStd::is_same<decltype(AZStd::get<1>(AZStd::move(testVariant))), const int64_t&&>::value,
                "AZStd::get should return const rvalue reference to const alternative for non-const variant");
            EXPECT_EQ(23LL, AZStd::get<1>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, IndexGetConstRValueVariantNonConstAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            
            static_assert(AZStd::is_same<decltype(AZStd::get<0>(AZStd::move(testVariant))), const int32_t&&>::value,
                "AZStd::get should return const rvalue reference to non-const alternative for const variant");
            EXPECT_EQ(6, AZStd::get<0>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, IndexGetConstRValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            const AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            
            static_assert(AZStd::is_same<decltype(AZStd::get<1>(AZStd::move(testVariant))), const int64_t&&>::value,
                "AZStd::get should return const rvalue reference to const alternative for const variant");
            EXPECT_EQ(23LL, AZStd::get<1>(AZStd::move(testVariant)));
        }

        // get<Type>
        TEST_F(VariantTest, TypeGetLValueVariantNonConstAlternativeSucceeds)
        {
            AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            EXPECT_EQ(6, AZStd::get<int32_t>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<int32_t>(testVariant)), int32_t&>::value,
                "AZStd::get should return lvalue reference to non-const alternative for non-const variant");
        }

        TEST_F(VariantTest, TypeGetLValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            EXPECT_EQ(23LL, AZStd::get<const int64_t>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<const int64_t>(testVariant)), const int64_t&>::value,
                "AZStd::get should return const lvalue reference to const alternativefor non-const variant");
        }

        TEST_F(VariantTest, TypeGetConstLValueVariantNonConstAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            EXPECT_EQ(6, AZStd::get<int32_t>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<int32_t>(testVariant)), const int32_t&>::value,
                "AZStd::get should return const lvalue reference to non-const alternative for const variant");
        }

        TEST_F(VariantTest, TypeGetConstLValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            const AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());
            EXPECT_EQ(23LL, AZStd::get<const int64_t>(testVariant));
            static_assert(AZStd::is_same<decltype(AZStd::get<const int64_t>(testVariant)), const int64_t&>::value,
                "AZStd::get should return const lvalue reference to const alternative for const variant");
        }

        TEST_F(VariantTest, TypeGetRValueVariantNonConstAlternativeSucceeds)
        {
            AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());
            static_assert(AZStd::is_same<decltype(AZStd::get<int32_t>(AZStd::move(testVariant))), int32_t&&>::value,
                "AZStd::get should return rvalue reference to non-const alternative for non-const variant");
            EXPECT_EQ(6, AZStd::get<int32_t>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, TypeGetRValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());

            static_assert(AZStd::is_same<decltype(AZStd::get<const int64_t>(AZStd::move(testVariant))), const int64_t&&>::value,
                "AZStd::get should return const rvalue reference to const alternative for non-const variant");
            EXPECT_EQ(23LL, AZStd::get<const int64_t>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, TypeGetConstRValueVariantNonConstAlternativeSucceeds)
        {
            const AZStd::variant<int32_t, const int64_t> testVariant(6);
            ASSERT_EQ(0, testVariant.index());

            static_assert(AZStd::is_same<decltype(AZStd::get<int32_t>(AZStd::move(testVariant))), const int32_t&&>::value,
                "AZStd::get should return const rvalue reference to non-const alternative for const variant");
            EXPECT_EQ(6, AZStd::get<int32_t>(AZStd::move(testVariant)));
        }

        TEST_F(VariantTest, TypeGetConstRValueVariantConstAlternativeSucceeds)
        {
            // Set const alternative on construct
            const AZStd::variant<int32_t, const int64_t> testVariant(static_cast<int64_t>(23LL));
            ASSERT_EQ(1, testVariant.index());

            static_assert(AZStd::is_same<decltype(AZStd::get<const int64_t>(AZStd::move(testVariant))), const int64_t&&>::value,
                "AZStd::get should return const rvalue reference to const alternative for const variant");
            EXPECT_EQ(23LL, AZStd::get<const int64_t>(AZStd::move(testVariant)));
        }
    }

    namespace VisitorQualifier
    {
        // non-const lvalue visitor
        TEST_F(VariantTest, VisitForwardsLValueMemberVisitorCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsLValueMemberVisitorCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsLValueMemberVisitorCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }

        // const lvalue visitor
        TEST_F(VariantTest, VisitForwardsConstLValueMemberVisitorCorrectly_WithSingleAlternativeVariant)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstLValueMemberVisitorCorrectly_WithMultipleAlternativeVariant)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstLValueMemberVisitorCorrectly_WithMultipleVariants)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }

        // non-const rvalue visitor
        TEST_F(VariantTest, VisitForwardsRValueMemberVisitorCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsRValueMemberVisitorCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsRValueMemberVisitorCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }

        // const rvalue visitor
        TEST_F(VariantTest, VisitForwardsConstRValueMemberVisitorCorrectly_WithSingleAlternativeVariant)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstRValueMemberVisitorCorrectly_WithMultiAlternativeVariant)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstRValueMemberVisitorCorrectly_WithMultipleVariants)
        {
            const VariantTestInternal::MemberFuncQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(AZStd::move(callable), testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }

        TEST_F(VariantTest, VisitWithExplicitReturnTypeSucceeds)
        {
            auto returnConstCharPtrVisitor = [](auto&& variantAlt) -> const char*
            {
                (void)variantAlt;
                return AZStd::is_same<AZStd::remove_cvref_t<decltype(variantAlt)>, const char*>::value ? "Boat" : "Willy";
            };

            AZStd::variant<float, const char*> testVariant1(static_cast<const char*>("Steam"));
            auto testResultData = AZStd::visit<AZStd::string_view>(returnConstCharPtrVisitor, testVariant1);
            EXPECT_FALSE(testResultData.empty());
            EXPECT_EQ("Boat", testResultData);
        }
    }

    namespace VariantQualifier
    {
        // non-const lvalue variant
        TEST_F(VariantTest, VisitForwardsLValueVariantCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsLValueVariantCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsLValueVariantCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::lvalue_ref, calledFunc);
        }

        // const lvalue variant
        TEST_F(VariantTest, VisitForwardsConstLValueVariantCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstLValueVariantCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant2);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstLValueVariantCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<float, double> testVariant3(2.0);
            const AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, testVariant3, testVariant4);
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_lvalue_ref, calledFunc);
        }

        // non-const rvalue variant
        TEST_F(VariantTest, VisitForwardsRValueVariantCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsRValueVariantCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant2));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsRValueVariantCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            AZStd::variant<float, double> testVariant3(2.0);
            const AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant3), AZStd::move(testVariant4));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::rvalue_ref, calledFunc);
        }

        // const rvalue variant
        TEST_F(VariantTest, VisitForwardsConstRValueVariantCorrectly_WithSingleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<int16_t> testVariant(static_cast<int16_t>(7));
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstRValueVariantCorrectly_WithMultipleAlternativeVariant)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<int16_t, AZStd::string> testVariant2("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant2));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }
        TEST_F(VariantTest, VisitForwardsConstRValueVariantCorrectly_WithMultipleVariants)
        {
            VariantTestInternal::ParameterQualifierCallable callable{};
            const AZStd::variant<float, double> testVariant3(2.0);
            const AZStd::variant<AZStd::string_view, const char*> testVariant4("ConstMember");
            VariantTestInternal::MemberFuncQualifiers calledFunc = AZStd::visit(callable, AZStd::move(testVariant3), AZStd::move(testVariant4));
            EXPECT_EQ(VariantTestInternal::MemberFuncQualifiers::const_rvalue_ref, calledFunc);
        }
    }

    TEST_F(VariantTest, VisitIsEvaluatedInConstexprContextSucceeds)
    {
        constexpr VariantTestInternal::ConstexprVisitorHelper constexprHelper{};
        using TestVariant = AZStd::variant<uint32_t>;
        // variant visiting ends up calling array operator [] which is constexpr but calls AZ_Assert which is not constexpr, so it cannot be solved in a static_assert
        constexpr TestVariant testVariant(21);
        static_assert(AZStd::visit(constexprHelper, testVariant) == 21, "Constexpr Variant not equal to expected result");
        EXPECT_EQ(21, AZStd::visit(constexprHelper, testVariant));
    }
    TEST_F(VariantTest, VisitWithMultipleAlternativeEvaluatedInConstexprContextSucceeds)
    {
        constexpr VariantTestInternal::ConstexprVisitorHelper constexprHelper{};
        using TestVariant = AZStd::variant<int16_t, int64_t, char>;
        // variant visiting ends up calling array operator [] which is constexpr but calls AZ_Assert which is not constexpr, so it cannot be solved in a static_assert
        constexpr TestVariant testVariant(static_cast<int64_t>(42LL));
        static_assert(AZStd::visit(constexprHelper, testVariant) == 42, "Constexpr Variant not equal to expected result");
        EXPECT_EQ(42, AZStd::visit(constexprHelper, testVariant));
    }
    TEST_F(VariantTest, VisitWithMultipleVariantEvaluatedInConstexprContextSucceeds)
    {
            using TestVariant1 = AZStd::variant<int32_t>;
            using TestVariant2 = AZStd::variant<int32_t, char*, int64_t>;
            using TestVariant3 = AZStd::variant<bool, uint32_t, int32_t>;
            // variant visiting ends up calling array operator [] which is constexpr but calls AZ_Assert which is not constexpr, so it cannot be solved in a static_assert
            constexpr TestVariant1 testVariant1;
            constexpr TestVariant2 testVariant2(nullptr);
            constexpr TestVariant3 testVariant3;
            static_assert(AZStd::visit(VariantTestInternal::VariadicVariantSizeof{}, testVariant1, testVariant2, testVariant3) == 3,
                "Sizeof visitor should return the same number of variants supplied to it");
            EXPECT_EQ(3, AZStd::visit(VariantTestInternal::VariadicVariantSizeof{}, testVariant1, testVariant2, testVariant3));
    }

    TEST_F(VariantTest, VisitorAcceptsNonConstLValueReferenceSucceeds)
    {
        AZStd::variant<VariantTestInternal::TrivialAlt> variantAlt;

        auto nonConstLRefVisitor = [](VariantTestInternal::TrivialAlt&)
        {
        };
        AZStd::visit(nonConstLRefVisitor, variantAlt);
    }

    TEST_F(VariantTest, VisitorAcceptsNonConstRValueReferenceSucceeds)
    {
        AZStd::variant<VariantTestInternal::TrivialAlt> variantAlt;

        auto nonConstRRefVisitor = [](VariantTestInternal::TrivialAlt&&)
        {
        };
        AZStd::visit(nonConstRRefVisitor, AZStd::move(variantAlt));
    }

    namespace HashFunctions
    {
        TEST_F(VariantTest, HashMonostateSucceeds)
        {
            AZStd::monostate testMonostate1;
            size_t resultHash1 = AZStd::hash<AZStd::monostate>{}(testMonostate1);
            const AZStd::monostate testMonostate2 {};
            size_t resultHash2 = AZStd::hash<AZStd::monostate>{}(testMonostate2);

            EXPECT_EQ(resultHash1, resultHash2);
        }

        TEST_F(VariantTest, HashVariantAlternativeOfSameTypeHashesToDifferentResults)
        {
            using TestVariant = AZStd::variant<AZStd::monostate, AZStd::monostate>;
            TestVariant testVariant1(AZStd::in_place_index_t<0>{});
            TestVariant testVariant2(AZStd::in_place_index_t<1>{});
            size_t resultHash1 = AZStd::hash<TestVariant>{}(testVariant1);
            size_t resultHash2 = AZStd::hash<TestVariant>{}(testVariant2);

            EXPECT_NE(resultHash1, resultHash2);
        }

        TEST_F(VariantTest, HashVariantDifferentValuesForSameAlternativeHashesToDifferentResults)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t, int64_t>;
            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 42);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 43);
            size_t resultHash1 = AZStd::hash<TestVariant>{}(testVariant1);
            size_t resultHash2 = AZStd::hash<TestVariant>{}(testVariant2);

            EXPECT_NE(resultHash1, resultHash2);
        }

        TEST_F(VariantTest, HashVariantCopyOfVariantHashesToSameResult)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t, int64_t>;
            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 47);
            TestVariant copyVariant(testVariant1);
            size_t resultHash1 = AZStd::hash<TestVariant>{}(testVariant1);
            size_t resultHash2 = AZStd::hash<TestVariant>{}(copyVariant);

            EXPECT_EQ(resultHash1, resultHash2);
        }

        TEST_F(VariantTest, HashVariantDefaultConstructedVariantHashesToSameValue)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t, int64_t>;
            TestVariant testVariant1;
            TestVariant testVariant2;
            size_t resultHash1 = AZStd::hash<TestVariant>{}(testVariant1);
            size_t resultHash2 = AZStd::hash<TestVariant>{}(testVariant2);

            EXPECT_EQ(resultHash1, resultHash2);
        }

        TEST_F(VariantTest, HashVariantHashRemainsStableForSameValue)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t, int64_t>;
            constexpr int64_t initialValue = 84;
            TestVariant testVariant1(AZStd::in_place_index_t<2>{}, initialValue);
            size_t expectedHash = AZStd::hash<TestVariant>{}(testVariant1);

            AZStd::get<2>(testVariant1) = 74;
            size_t resultHash = AZStd::hash<TestVariant>{}(testVariant1);
            EXPECT_NE(expectedHash, resultHash);

            AZStd::get<2>(testVariant1) = initialValue;
            resultHash = AZStd::hash<TestVariant>{}(testVariant1);
            EXPECT_EQ(expectedHash, resultHash);
        }
    }

    namespace RelationalOperators
    {
        // equal
        TEST_F(VariantTest, OperatorEqualForVariantWithSameAlternativeAndValueSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 7);

            EXPECT_TRUE(testVariant1 == testVariant2);
        }

        TEST_F(VariantTest, OperatorEqualForVariantWithSameAlternativeAndDifferentValueFails)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 8);

            EXPECT_FALSE(testVariant1 == testVariant2);
        }
        TEST_F(VariantTest, OperatorEqualForVariantWithDifferentAlternativeFails)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 7);

            EXPECT_FALSE(testVariant1 == testVariant2);
        }

        // not-equal
        TEST_F(VariantTest, OperatorNotEqualForVariantWithSameAlternativeAndValueFails)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 7);

            EXPECT_FALSE(testVariant1 != testVariant2);
        }

        TEST_F(VariantTest, OperatorNotEqualForVariantWithSameAlternativeAndDifferentValueSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 8);

            EXPECT_TRUE(testVariant1 != testVariant2);
        }
        TEST_F(VariantTest, OperatorNotEqualForVariantWithDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<int8_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, static_cast<int8_t>(7));
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 7);

            EXPECT_TRUE(testVariant1 != testVariant2);
        }

        // less
        TEST_F(VariantTest, OperatorLessForVariantWithSameAlternativeAndValueFails)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 7);

            EXPECT_FALSE(testVariant1 < testVariant2);
        }
        TEST_F(VariantTest, OperatorLessForVariantWithSameAlternativeAndDifferentValueSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 8);

            EXPECT_TRUE(testVariant1 < testVariant2);
            EXPECT_FALSE(testVariant2 < testVariant1);
        }
        TEST_F(VariantTest, OperatorLessForVariantWithDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 10000);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 7);

            EXPECT_TRUE(testVariant1 < testVariant2);
            EXPECT_FALSE(testVariant2 < testVariant1);
        }

        // greater
        TEST_F(VariantTest, OperatorGreaterForVariantWithSameAlternativeAndValueFails)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 7);

            EXPECT_FALSE(testVariant1 > testVariant2);
        }
        TEST_F(VariantTest, OperatorGreaterForVariantWithSameAlternativeAndDifferentValueSucceeds)
        {
            using TestVariant = AZStd::variant<int8_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, 427);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 321);

            EXPECT_TRUE(testVariant1 > testVariant2);
            EXPECT_FALSE(testVariant2 > testVariant1);
        }
        TEST_F(VariantTest, OperatorGreaterForVariantWithDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, -5000);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 17);

            EXPECT_TRUE(testVariant1 > testVariant2);
            EXPECT_FALSE(testVariant2 > testVariant1);
        }

        // less equal
        TEST_F(VariantTest, OperatorLessEqualForVariantWithSameAlternativeAndValueSucceeds)
        {
            using TestVariant = AZStd::variant<int64_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, 7);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, 7);

            EXPECT_TRUE(testVariant1 <= testVariant2);
        }
        TEST_F(VariantTest, OperatorLessEqualForVariantWithSameAlternativeAndDifferentValueSucceeds)
        {
            using TestVariant = AZStd::variant<int8_t, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, 19);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 21);
            TestVariant testVariant3(AZStd::in_place_index_t<1>{}, 21);

            EXPECT_TRUE(testVariant1 <= testVariant2);
            EXPECT_FALSE(testVariant2 <= testVariant1);
            EXPECT_TRUE(testVariant2 <= testVariant3);
        }
        TEST_F(VariantTest, OperatorLessEqualForVariantWithDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<bool, float>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, false);
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, 16.0f);

            EXPECT_TRUE(testVariant1 <= testVariant2);
            EXPECT_FALSE(testVariant2 <= testVariant1);
        }

        // greater equal
        TEST_F(VariantTest, OperatorGreaterEqualForVariantWithSameAlternativeAndValueSucceeds)
        {
            using TestVariant = AZStd::variant<AZStd::string, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, "Hello");
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, "Hello");

            EXPECT_TRUE(testVariant1 >= testVariant2);
        }
        TEST_F(VariantTest, OperatorGreaterEqualForVariantWithSameAlternativeAndDifferentValueSucceeds)
        {
            using TestVariant = AZStd::variant<int16_t, AZStd::string>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, "World");
            TestVariant testVariant2(AZStd::in_place_index_t<1>{}, "Worlc");
            TestVariant testVariant3(AZStd::in_place_index_t<1>{}, "Worlc");

            EXPECT_TRUE(testVariant1 >= testVariant2);
            EXPECT_FALSE(testVariant2 >= testVariant1);
            EXPECT_TRUE(testVariant2 >= testVariant3);
        }
        TEST_F(VariantTest, OperatorGreaterEqualForVariantWithDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<void*, float>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, 34.5f);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, nullptr);

            EXPECT_TRUE(testVariant1 >= testVariant2);
            EXPECT_FALSE(testVariant2 >= testVariant1);
        }
    }

    namespace VariantSwap
    {
        TEST_F(VariantTest, SwapSameAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<AZStd::string, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<0>{}, "Hello");
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, "World");
            testVariant1.swap(testVariant2);
            EXPECT_EQ("World", AZStd::get<0>(testVariant1));
            EXPECT_EQ("Hello", AZStd::get<0>(testVariant2));
        }

        TEST_F(VariantTest, SwapDifferentAlternativeSucceeds)
        {
            using TestVariant = AZStd::variant<AZStd::string, int32_t>;

            TestVariant testVariant1(AZStd::in_place_index_t<1>{}, 501);
            TestVariant testVariant2(AZStd::in_place_index_t<0>{}, "TestCode");
            testVariant1.swap(testVariant2);
            ASSERT_EQ(0, testVariant1.index());
            EXPECT_EQ("TestCode", AZStd::get<0>(testVariant1));

            ASSERT_EQ(1, testVariant2.index());
            EXPECT_EQ(501, AZStd::get<1>(testVariant2));
        }

        TEST_F(VariantTest, SwapWithMoveOnlyTypeSucceeds)
        {
            using TestVariant = AZStd::variant<VariantTestInternal::MoveOnly>;

            TestVariant testVariant1;
            AZStd::get<0>(testVariant1).m_value = 10;
            TestVariant testVariant2;
            AZStd::get<0>(testVariant2).m_value = 20;
            testVariant1.swap(testVariant2);
            EXPECT_EQ(20, AZStd::get<0>(testVariant1).m_value);
            EXPECT_EQ(10, AZStd::get<0>(testVariant2).m_value);
            EXPECT_NE(0, AZStd::get<0>(testVariant1).m_moveAssignmentCounter);
            EXPECT_NE(0, AZStd::get<0>(testVariant2).m_moveAssignmentCounter);
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Written with help from libcxx's any tests
// https://github.com/llvm-mirror/libcxx/tree/7175a079211ec78c8232d9d55fa4c1f9eeae803d/test/std/experimental/any

#include <AzCore/std/any.h>

#include "UserTypes.h"

using AZStd::any;
using AZStd::any_cast;
using AZStd::any_numeric_cast;

namespace UnitTest
{
    using Small0 = CreationCounter<8, 0>;
    using Small1 = CreationCounter<8, 1>;
    using Large0 = CreationCounter<256, 0>;
    using Large1 = CreationCounter<256, 1>;
    using Align0 = CreationCounter<8, 0, 64>;
    using Align1 = CreationCounter<8, 1, 64>;
}

// Specialize for the type's we're using to get comprehensible output
namespace testing
{
    namespace internal
    {
#define DEF_GTN_SINGLE(Struct) template<> std::string GetTypeName<::UnitTest::Struct>() { return #Struct; }
#define DEF_GTN_PAIR(Struct1, Struct2) template<> std::string GetTypeName<AZStd::pair<::UnitTest::Struct1, ::UnitTest::Struct2>>() { return #Struct1 ", " #Struct2; }

        DEF_GTN_SINGLE(Small0);
        DEF_GTN_SINGLE(Large0);
        DEF_GTN_SINGLE(Align0);

        DEF_GTN_PAIR(Small0, Small1);
        DEF_GTN_PAIR(Large0, Large1);
        DEF_GTN_PAIR(Align0, Align1);
        DEF_GTN_PAIR(Small0, Large0);
        DEF_GTN_PAIR(Small0, Align0);
        DEF_GTN_PAIR(Large0, Small0);
        DEF_GTN_PAIR(Large0, Align0);
        DEF_GTN_PAIR(Align0, Small0);
        DEF_GTN_PAIR(Align0, Large0);

#undef DEF_GTN_SINGLE
#undef DEF_GTN_PAIR
    }
}

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class AnyTest
        : public LeakDetectionFixture
    { };

    // Fixture for tests with 1 type
    template<typename TestStruct>
    class AnySizedTest
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            TestStruct::Reset();
        }
    };
    using AnySizedTestTypes = ::testing::Types<Small0, Large0, Align0>;
    TYPED_TEST_CASE(AnySizedTest, AnySizedTestTypes);

    // Fixture for tests with 2 types (for converting between types)
    template <typename StructPair>
    class AnyConversionTest
        : public LeakDetectionFixture
    {
    public:
        using LHS = typename StructPair::first_type;
        using RHS = typename StructPair::second_type;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            LHS::Reset();
            RHS::Reset();
        }
    };
    using AnyConversionTestTypes = ::testing::Types <
        AZStd::pair<Small0, Small1>, // Small -> Small
        AZStd::pair<Large0, Large1>, // Large -> Large
        AZStd::pair<Align0, Align1>, // Align -> Align
        AZStd::pair<Small0, Large0>, // Small -> Large
        AZStd::pair<Small0, Align0>, // Small -> Align
        AZStd::pair<Large0, Small0>, // Large -> Small
        AZStd::pair<Large0, Align0>, // Large -> Align
        AZStd::pair<Align0, Small0>, // Align -> Small
        AZStd::pair<Align0, Large0>  // Align -> Large
    >;
    TYPED_TEST_CASE(AnyConversionTest, AnyConversionTestTypes);

    //////////////////////////////////////////////////////////////////////////
    // Tests for constructors
    namespace Constructor
    {
        // Construct empty
        TEST_F(AnyTest, Any_EmptyConstruct_IsEmpty)
        {
            const any a;
            ASSERT_TRUE(a.empty());
        }

        // Construct via copy value
        TYPED_TEST(AnySizedTest, Any_ConstructFromData_Copy)
        {
            {
                TypeParam t;
                EXPECT_EQ(TypeParam::s_count, 1);

                any any1(t);
                EXPECT_EQ(TypeParam::s_count, 2);
                EXPECT_EQ(TypeParam::s_copied, 1);
                EXPECT_EQ(TypeParam::s_moved, 0);
            }
        }

        // Construct via move value
        TYPED_TEST(AnySizedTest, Any_ConstructFromData_Move)
        {
            {
                TypeParam t;
                EXPECT_EQ(TypeParam::s_count, 1);

                any any1(AZStd::move(t));
                EXPECT_EQ(TypeParam::s_count, 2);
                EXPECT_EQ(TypeParam::s_copied, 0);
                EXPECT_EQ(TypeParam::s_moved, 1);
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Copy empty
        TEST_F(AnyTest, Any_CopyConstructEmpty_IsEmpty)
        {
            const any any1;
            const any any2(any1);

            EXPECT_TRUE(any1.empty());
            EXPECT_TRUE(any2.empty());
        }

        // Copy with data
        TYPED_TEST(AnySizedTest, Any_CopyConstructValid_IsValid)
        {
            any any1(TypeParam(36));

            EXPECT_EQ(TypeParam::s_count, 1);
            EXPECT_EQ(TypeParam::s_copied, 0);
            EXPECT_EQ(TypeParam::s_moved, 1);
            EXPECT_EQ(any_cast<TypeParam&>(any1).val(), 36);

            any any2(any1);

            EXPECT_EQ(TypeParam::s_copied, 1);
            EXPECT_EQ(TypeParam::s_count, 2);
            EXPECT_EQ(TypeParam::s_moved, 1);

            // Modify a and check that any2 is unchanged
            any_cast<TypeParam&>(any1).val() = -1;
            EXPECT_EQ(any_cast<TypeParam&>(any1).val(), -1);
            EXPECT_EQ(any_cast<TypeParam&>(any2).val(), 36);

            // modify any2 and check that any1 is unchanged
            any_cast<TypeParam&>(any2).val() = 75;
            EXPECT_EQ(any_cast<TypeParam&>(any1).val(), -1);
            EXPECT_EQ(any_cast<TypeParam&>(any2).val(), 75);

            // clear a and check that a2 is unchanged
            any1.clear();
            EXPECT_TRUE(any1.empty());
            EXPECT_EQ(any_cast<TypeParam&>(any2).val(), 75);
        }

        // Move empty
        TEST_F(AnyTest, Any_MoveConstructEmpty_IsEmpty)
        {
            const any any1;
            const any any2(AZStd::move(any1));

            EXPECT_TRUE(any1.empty());
            EXPECT_TRUE(any2.empty());
        }

        // Move with data
        TYPED_TEST(AnySizedTest, Any_MoveConstructValid_IsValid)
        {
            any any1(TypeParam(42));
            EXPECT_EQ(TypeParam::s_count, 1);
            EXPECT_EQ(TypeParam::s_copied, 0);
            EXPECT_EQ(TypeParam::s_moved, 1);

            any any2(std::move(any1));

            EXPECT_GT(TypeParam::s_moved, 1); // zero or more move operations can be performed.
            EXPECT_EQ(TypeParam::s_copied, 0); // no copies can be performed.
            EXPECT_EQ(TypeParam::s_count, 1); // Only one instance may remain.
            EXPECT_TRUE(any1.empty()); // Moves are always destructive.
            EXPECT_EQ(any_cast<TypeParam&>(any2).val(), 42);
        }

        // Forward with data
        TYPED_TEST(AnySizedTest, Any_ForwardConstructValid_IsValid)
        {
            any any1(AZStd::in_place_type_t<TypeParam>(), 55);
            EXPECT_EQ(1, TypeParam::s_count);
            EXPECT_EQ(0, TypeParam::s_copied);
            EXPECT_EQ(0, TypeParam::s_moved);

            any any2(AZStd::in_place_type_t<TypeParam>(), AZStd::initializer_list<int>{ 55 });

            EXPECT_EQ(2, TypeParam::s_count);
            EXPECT_EQ(0, TypeParam::s_moved);
            EXPECT_EQ(0, TypeParam::s_copied);
            EXPECT_EQ(any_cast<TypeParam&>(any1).val(), any_cast<TypeParam&>(any2).val());
        }

        // make_any helper forward functions
        TYPED_TEST(AnySizedTest, Any_MakeAnyForwarder_IsValid)
        {
            any any1 = AZStd::make_any<TypeParam>(24);
            EXPECT_EQ(1, TypeParam::s_count);
            EXPECT_EQ(0, TypeParam::s_copied);
            EXPECT_EQ(0, TypeParam::s_moved);

            any any2 = AZStd::make_any<TypeParam, int>({ 24 });

            EXPECT_EQ(2, TypeParam::s_count);
            EXPECT_EQ(0, TypeParam::s_moved);
            EXPECT_EQ(0, TypeParam::s_copied);
            EXPECT_EQ(any_cast<TypeParam&>(any1).val(), any_cast<TypeParam&>(any2).val());
        }

        template <typename ValueType>
        void InplaceAnyTypeInfo(AZStd::any::Action action, any* dest, const any*)
        {
            switch (action)
            {
            case any::Action::Reserve:
                // If doing small buffer optimization, no need to validate anything
                if (dest->get_type_info().m_useHeap)
                {
                    // Allocate space for object on heap
                    AZStd::allocator systemAllocator;
                    *reinterpret_cast<void**>(dest) = systemAllocator.allocate(sizeof(ValueType), alignof(ValueType));
                }
                break;
            case any::Action::Construct:
                break;
            case any::Action::Copy:
            case any::Action::Move:
                ADD_FAILURE() << "Copy/Move Constructor should not be getting invoke in the InPlace test";
                break;
            case any::Action::Destruct:
                break;
            case any::Action::Destroy:

                // Call the destructor
                reinterpret_cast<ValueType*>(AZStd::any_cast<void>(dest))->~ValueType();

                // Clear memory
                if (dest->get_type_info().m_useHeap)
                {
                    AZStd::allocator systemAllocator;
                    systemAllocator.deallocate(AZStd::any_cast<void>(dest), sizeof(ValueType), alignof(ValueType));
                }
                break;
            default:
                ADD_FAILURE() << "Default case should never get invoked";
            }
        }

        TEST_F(AnyTest, Any_CustomTypeInfoConstructorWithInplace_IsValid)
        {
            using VectorType = AZStd::vector<int>;
            AZStd::any::type_info vectorTypeInfo;
            vectorTypeInfo.m_id = azrtti_typeid<VectorType>();
            vectorTypeInfo.m_isPointer = false;
            vectorTypeInfo.m_useHeap = sizeof(VectorType) > AZStd::Internal::ANY_SBO_BUF_SIZE;
            vectorTypeInfo.m_handler = &InplaceAnyTypeInfo<VectorType>;
            any vectorAny(vectorTypeInfo, AZStd::in_place_type_t<VectorType>{}, 3, 17);
            EXPECT_TRUE(vectorAny.is<VectorType>());
            VectorType& vectorRef = AZStd::any_cast<VectorType&>(vectorAny);
            EXPECT_EQ(3, vectorRef.size());
            EXPECT_EQ(17, vectorRef[0]);
            EXPECT_EQ(17, vectorRef[1]);
            EXPECT_EQ(17, vectorRef[2]);
        }

        TEST_F(AnyTest, Any_CustomTypeInfoConstructorWithInitializerListAndInplace_IsValid)
        {
            using VectorType = AZStd::vector<int>;
            AZStd::any::type_info vectorTypeInfo;
            vectorTypeInfo.m_id = azrtti_typeid<VectorType>();
            vectorTypeInfo.m_isPointer = false;
            vectorTypeInfo.m_useHeap = sizeof(VectorType) > AZStd::Internal::ANY_SBO_BUF_SIZE;
            vectorTypeInfo.m_handler = &InplaceAnyTypeInfo<VectorType>;
            any vectorAny(vectorTypeInfo, AZStd::in_place_type_t<VectorType>{}, { 1, 2, 3, 4 });
            EXPECT_TRUE(vectorAny.is<VectorType>());
            VectorType& vectorRef = AZStd::any_cast<VectorType&>(vectorAny);
            EXPECT_EQ(4, vectorRef.size());
            EXPECT_EQ(1, vectorRef[0]);
            EXPECT_EQ(2, vectorRef[1]);
            EXPECT_EQ(3, vectorRef[2]);
            EXPECT_EQ(4, vectorRef[3]);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Tests for assignment operator
    namespace Assignment
    {
        // Test copy assign other any
        TYPED_TEST(AnyConversionTest, Any_CopyAssignAny_IsValid)
        {
            using LHS = typename TestFixture::LHS;
            using RHS = typename TestFixture::RHS;
            {
                any lhs(LHS(1));
                any const rhs(RHS(2));

                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 1);
                EXPECT_EQ(RHS::s_copied, 0);

                lhs = rhs;

                EXPECT_EQ(RHS::s_copied, 1);
                EXPECT_EQ(LHS::s_count, 0);
                EXPECT_EQ(RHS::s_count, 2);

                EXPECT_EQ(any_cast<const RHS&>(lhs).val(), 2);
                EXPECT_EQ(any_cast<const RHS&>(rhs).val(), 2);
            }

            EXPECT_EQ(LHS::s_count, 0);
            EXPECT_EQ(RHS::s_count, 0);
        }

        // Test move assign other any
        TYPED_TEST(AnyConversionTest, Any_MoveAssignAny_IsValid)
        {
            using LHS = typename TestFixture::LHS;
            using RHS = typename TestFixture::RHS;
            {
                any lhs(LHS(1));
                any rhs(RHS(2));

                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 1);
                EXPECT_EQ(RHS::s_moved, 1);

                lhs = AZStd::move(rhs);

                EXPECT_GT(RHS::s_moved, 1);
                EXPECT_EQ(RHS::s_copied, 0);
                EXPECT_EQ(LHS::s_count, 0);
                EXPECT_EQ(RHS::s_count, 1);

                EXPECT_EQ(any_cast<RHS&>(lhs).val(), 2);
                EXPECT_TRUE(rhs.empty());
            }

            EXPECT_EQ(LHS::s_count, 0);
            EXPECT_EQ(RHS::s_count, 0);
        }

        // Test copy assign other any
        TYPED_TEST(AnySizedTest, Any_CopyAssignValue_IsValid)
        {
            {
                any lhs;
                TypeParam rhs(42);
                EXPECT_EQ(TypeParam::s_count, 1);
                EXPECT_EQ(TypeParam::s_copied, 0);

                lhs = rhs;

                EXPECT_EQ(TypeParam::s_count, 2);
                EXPECT_EQ(TypeParam::s_copied, 1);
                EXPECT_GT(TypeParam::s_moved, 0);

                EXPECT_EQ(any_cast<const TypeParam&>(lhs).val(), 42);
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test move assign other any
        TYPED_TEST(AnySizedTest, Any_MoveAssignValue_IsValid)
        {
            {
                any lhs;
                TypeParam rhs(42);
                EXPECT_EQ(TypeParam::s_count, 1);
                EXPECT_EQ(TypeParam::s_moved, 0);

                lhs = AZStd::move(rhs);

                EXPECT_EQ(TypeParam::s_count, 2);
                EXPECT_EQ(TypeParam::s_copied, 0);
                EXPECT_GT(TypeParam::s_moved, 1);
                EXPECT_EQ(any_cast<TypeParam&>(lhs).val(), 42);
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test copy assign other any, and that old value is destroyed
        TYPED_TEST(AnyConversionTest, Any_CopyAssignAny_OldIsDestroyed)
        {
            using LHS = typename TestFixture::LHS;
            using RHS = typename TestFixture::RHS;
            {
                any lhs(LHS(1));
                any const rhs(RHS(2));

                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 1);
                EXPECT_EQ(RHS::s_copied, 0);

                lhs = rhs;

                EXPECT_EQ(RHS::s_copied, 1);
                EXPECT_EQ(LHS::s_count, 0);
                EXPECT_EQ(RHS::s_count, 2);

                EXPECT_EQ(any_cast<const RHS&>(lhs).val(), 2);
                EXPECT_EQ(any_cast<const RHS&>(rhs).val(), 2);
            }

            EXPECT_EQ(LHS::s_count, 0);
            EXPECT_EQ(RHS::s_count, 0);
        }

        // Test copy assign empty any, and that old value is destroyed
        TYPED_TEST(AnySizedTest, Any_CopyAssignEmptyAny_OldIsDestroyed)
        {
            {
                any lhs(TypeParam(1));
                any const rhs;

                EXPECT_EQ(TypeParam::s_count, 1);
                EXPECT_EQ(TypeParam::s_copied, 0);

                lhs = rhs;

                EXPECT_EQ(TypeParam::s_copied, 0);
                EXPECT_EQ(TypeParam::s_count, 0);

                EXPECT_TRUE(lhs.empty());
                EXPECT_TRUE(rhs.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test copy assign empty self any
        TEST_F(AnyTest, Any_CopyAssignSelfEmpty_IsEmpty)
        {
            any a;
            AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
            a = a;
            AZ_POP_DISABLE_WARNING
            EXPECT_TRUE(a.empty());
        }

        // Test copy assign self any
        TYPED_TEST(AnySizedTest, Any_CopyAssignSelf_IsNoop)
        {
            {
                any a((TypeParam(1)));
                EXPECT_EQ(TypeParam::s_count, 1);

                AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
                a = a;
                AZ_POP_DISABLE_WARNING

                EXPECT_EQ(TypeParam::s_count, 1);
                EXPECT_EQ(any_cast<const TypeParam&>(a).val(), 1);
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test move assign other any, and that the old value is destroyed
        TYPED_TEST(AnyConversionTest, Any_MoveAssignAny_OldIsDestroyed)
        {
            using LHS = typename TestFixture::LHS;
            using RHS = typename TestFixture::RHS;
            {
                LHS const s1(1);
                any a(s1);
                RHS const s2(2);
                any a2(s2);

                EXPECT_EQ(LHS::s_count, 2);
                EXPECT_EQ(RHS::s_count, 2);

                a = AZStd::move(a2);

                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 2);

                EXPECT_EQ(any_cast<const RHS&>(a).val(), 2);
                EXPECT_TRUE(a2.empty());
            }

            EXPECT_EQ(LHS::s_count, 0);
            EXPECT_EQ(RHS::s_count, 0);
        }

        // Test move assign other any over empty
        TYPED_TEST(AnySizedTest, Any_MoveAssignAny_IsValid)
        {
            {
                any a;
                any a2((TypeParam(1)));

                EXPECT_EQ(TypeParam::s_count, 1);

                a = std::move(a2);

                EXPECT_EQ(TypeParam::s_count, 1);

                EXPECT_EQ(any_cast<const TypeParam&>(a).val(), 1);
                EXPECT_TRUE(a2.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test move assign other empty any
        TYPED_TEST(AnySizedTest, Any_MoveAssignEmptyAny_OldIsDestroyed)
        {
            {
                any a((TypeParam(1)));
                any a2;

                EXPECT_EQ(TypeParam::s_count, 1);

                a = std::move(a2);

                EXPECT_EQ(TypeParam::s_count, 0);

                EXPECT_TRUE(a.empty());
                EXPECT_TRUE(a2.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }
    }

    namespace Modifiers
    {
        // Test clear empty any
        TEST_F(AnyTest, Any_ClearEmptyAny_IsEmpty)
        {
            any a;

            a.clear();

            EXPECT_TRUE(a.empty());
        }

        // Test clear valid any
        TYPED_TEST(AnySizedTest, Any_ClearValidAny_OldIsDestroyed)
        {
            {
                any a((TypeParam(1)));
                EXPECT_EQ(TypeParam::s_count, 1);
                EXPECT_EQ(any_cast<const TypeParam&>(a).val(), 1);

                a.clear();

                EXPECT_TRUE(a.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test swap 2 valid anys
        TYPED_TEST(AnyConversionTest, Any_SwapValidAnys_IsValid)
        {
            using LHS = typename TestFixture::LHS;
            using RHS = typename TestFixture::RHS;
            {
                any a1((LHS(1)));
                any a2(RHS(2));
                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 1);

                a1.swap(a2);

                EXPECT_EQ(LHS::s_count, 1);
                EXPECT_EQ(RHS::s_count, 1);

                EXPECT_EQ(any_cast<const RHS&>(a1).val(), 2);
                EXPECT_EQ(any_cast<const RHS&>(a2).val(), 1);
            }

            EXPECT_EQ(LHS::s_count, 0);
            EXPECT_EQ(RHS::s_count, 0);
        }

        // Test swap valid any with empty any
        TYPED_TEST(AnySizedTest, Any_SwapEmptyAndValidAny_IsValid)
        {
            {
                any a1((TypeParam(1)));
                any a2;
                EXPECT_EQ(TypeParam::s_count, 1);

                a1.swap(a2);

                EXPECT_EQ(TypeParam::s_count, 1);

                EXPECT_EQ(any_cast<const TypeParam&>(a2).val(), 1);
                EXPECT_TRUE(a1.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }

        // Test swap empty any with valid any
        TYPED_TEST(AnySizedTest, Any_SwapValidAndEmptyAny_IsValid)
        {
            {
                any a1((TypeParam(1)));
                any a2;
                EXPECT_EQ(TypeParam::s_count, 1);

                a2.swap(a1);

                EXPECT_EQ(TypeParam::s_count, 1);

                EXPECT_EQ(any_cast<const TypeParam&>(a2).val(), 1);
                EXPECT_TRUE(a1.empty());
            }

            EXPECT_EQ(TypeParam::s_count, 0);
        }
    }

    namespace Observers
    {
        // Test empty any is empty
        TEST_F(AnyTest, Any_EmptyAny_IsEmpty)
        {
            any a;
            EXPECT_TRUE(a.empty());
        }

        // Test valid any is not empty
        TYPED_TEST(AnySizedTest, Any_ValidAny_NotIsEmpty)
        {
            TypeParam const s(1);
            any a(s);
            EXPECT_FALSE(a.empty());
        }

        // Test empty any is empty
        TEST_F(AnyTest, Any_EmptyAny_IsTypeEmpty)
        {
            any const a;
            EXPECT_TRUE(a.type().IsNull());
        }

        // Test empty any is empty
        TYPED_TEST(AnySizedTest, Any_ValidAny_IsTypeValid)
        {
            TypeParam const s(1);
            any const a(s);
            EXPECT_EQ(a.type(), azrtti_typeid<TypeParam>());
        }

        namespace NonMembers
        {
            // Test swapping anys works
            TEST_F(AnyTest, Any_AzstdSwapAnys_IsValid)
            {
                any a1(1);
                any a2(2);

                AZStd::swap(a1, a2);

                EXPECT_EQ(any_cast<int>(a1), 2);
                EXPECT_EQ(any_cast<int>(a2), 1);
            }

            namespace AnyCast
            {
                // Test return types match
                TEST_F(AnyTest, Any_PointerAnyCast_IsReturnTypeValid)
                {
                    any a;
                    static_assert((AZStd::is_same<decltype(any_cast<int>(&a)), int*>::value), "Return type mismatch");
                    static_assert((AZStd::is_same<decltype(any_cast<int const>(&a)), int const*>::value), "Return type mismatch");

                    any const& ca = a;
                    (void)ca;
                    static_assert((AZStd::is_same<decltype(any_cast<int>(&ca)), int const*>::value), "Return type mismatch");
                    static_assert((AZStd::is_same<decltype(any_cast<int const>(&ca)), int const*>::value), "Return type mismatch");
                }

                // Test any_cast<...>(nullptr) always returns nullptr
                TEST_F(AnyTest, Any_PointerAnyCastNullptr_ReturnsNullptr)
                {
                    any* a = nullptr;
                    EXPECT_EQ(nullptr, any_cast<int>(a));
                    EXPECT_EQ(nullptr, any_cast<int const>(a));

                    any const* ca = nullptr;
                    EXPECT_EQ(nullptr, any_cast<int>(ca));
                    EXPECT_EQ(nullptr, any_cast<int const>(ca));
                }

                // Test any_cast(&emptyAny) always returns nullptr
                TEST_F(AnyTest, Any_PointerAnyCastEmptyAny_ReturnsNullptr)
                {
                    {
                        any a;
                        EXPECT_EQ(nullptr, any_cast<int>(&a));
                        EXPECT_EQ(nullptr, any_cast<int const>(&a));

                        any const& ca = a;
                        EXPECT_EQ(nullptr, any_cast<int>(&ca));
                        EXPECT_EQ(nullptr, any_cast<int const>(&ca));
                    }

                    // Create as non-empty, then make empty and run test.
                    {
                        any a(42);
                        a.clear();
                        EXPECT_EQ(nullptr, any_cast<int>(&a));
                        EXPECT_EQ(nullptr, any_cast<int const>(&a));

                        any const& ca = a;
                        EXPECT_EQ(nullptr, any_cast<int>(&ca));
                        EXPECT_EQ(nullptr, any_cast<int const>(&ca));
                    }
                }

                // Test any_cast(&validAny) always returns proper value
                TYPED_TEST(AnySizedTest, Any_PointerAnyCastValidAny_ReturnsValue)
                {
                    {
                        any a((TypeParam(42)));
                        any const& ca = a;
                        EXPECT_EQ(TypeParam::s_count, 1);
                        EXPECT_EQ(TypeParam::s_copied, 0);
                        EXPECT_EQ(TypeParam::s_moved, 1);

                        // Try a cast to a bad type.
                        // NOTE: Type cannot be an int.
                        EXPECT_EQ(any_cast<int>(&a), nullptr);
                        EXPECT_EQ(any_cast<int const>(&a), nullptr);

                        // Try a cast to the right type, but as a pointer.
                        EXPECT_EQ(any_cast<TypeParam*>(&a), nullptr);
                        EXPECT_EQ(any_cast<TypeParam const*>(&a), nullptr);

                        // Check getting a unqualified type from a non-const any.
                        TypeParam* v = any_cast<TypeParam>(&a);
                        EXPECT_NE(v, nullptr);
                        EXPECT_EQ(v->val(), 42);

                        // change the stored value and later check for the new value.
                        v->val() = 999;

                        // Check getting a const qualified type from a non-const any.
                        TypeParam const* cv = any_cast<TypeParam const>(&a);
                        EXPECT_NE(cv, nullptr);
                        EXPECT_EQ(cv, v);
                        EXPECT_EQ(cv->val(), 999);

                        // Check getting a unqualified type from a const any.
                        cv = any_cast<TypeParam>(&ca);
                        EXPECT_NE(cv, nullptr);
                        EXPECT_EQ(cv, v);
                        EXPECT_EQ(cv->val(), 999);

                        // Check getting a const-qualified type from a const any.
                        cv = any_cast<TypeParam const>(&ca);
                        EXPECT_NE(cv, nullptr);
                        EXPECT_EQ(cv, v);
                        EXPECT_EQ(cv->val(), 999);

                        // Check that no more objects were created, copied or moved.
                        EXPECT_EQ(TypeParam::s_count, 1);
                        EXPECT_EQ(TypeParam::s_copied, 0);
                        EXPECT_EQ(TypeParam::s_moved, 1);
                    }

                    EXPECT_EQ(TypeParam::s_count, 0);
                }
            }

            namespace AnyNumericCast
            {
                TEST_F(AnyTest, AnyNumericCast_Nullptr_ReturnsNullptr)
                {
                    {
                        any* a = nullptr;
                        int i;
                        EXPECT_FALSE(any_numeric_cast(a, i));
                    }

                    {
                        const any* a = nullptr;
                        int i;
                        EXPECT_FALSE(any_numeric_cast(a, i));
                    }
                }

#define EXPECT_ANY_IS(any_ptr, Type, value) do { Type _r; EXPECT_TRUE(any_numeric_cast<Type>(any_ptr, _r)); EXPECT_EQ(value, _r); } while(false)

                TEST_F(AnyTest, AnyNumericCast_SameType_ReturnsValid)
                {
                    any a;

                    a = 10.0f;
                    EXPECT_ANY_IS(&a, float, 10.0f);

                    a = 10.0;
                    EXPECT_ANY_IS(&a, double, 10.0);

                    a = int(10);
                    EXPECT_ANY_IS(&a, int, 10);
                }

                TEST_F(AnyTest, AnyNumericCast_FloatingPoint_ConversionsWork)
                {
                    any a;

                    a = 10.0f;
                    EXPECT_ANY_IS(&a, float, 10.0f);
                    EXPECT_ANY_IS(&a, double, 10.0);
                }

                TEST_F(AnyTest, AnyNumericCast_Integral_ConversionsWork)
                {
                    any a;

                    a = int(10);
                    EXPECT_ANY_IS(&a, int, 10);
                    EXPECT_ANY_IS(&a, long, 10);
                    EXPECT_ANY_IS(&a, char, 10);
                    EXPECT_ANY_IS(&a, unsigned int, 10u);
                    EXPECT_ANY_IS(&a, unsigned long, 10u);
                    EXPECT_ANY_IS(&a, unsigned char, 10u);
                }

                TEST_F(AnyTest, AnyNumericCast_Integral_AssertsOnDataLoss)
                {
                    any a;

                    {
                        // Test assert on signed -> unsigned conversion
                        a = -1;
                        AZ_TEST_START_TRACE_SUPPRESSION;
                        unsigned int v;
                        any_numeric_cast(&a, v);
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                    }

                    {
                        // Test assert on out of range
                        a = std::numeric_limits<int>::max();
                        AZ_TEST_START_TRACE_SUPPRESSION;
                        char v;
                        any_numeric_cast(&a, v);
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                    }
                }

                TEST_F(AnyTest, AnyNumericCast_IntegralToFloatingPoint_ConversionsWork)
                {
                    any a;

                    a = int(10);
                    EXPECT_ANY_IS(&a, int, 10);

                    EXPECT_ANY_IS(&a, float, 10.0f);
                    EXPECT_ANY_IS(&a, double, 10.0);
                }

                TEST_F(AnyTest, AnyNumericCast_FloatingPointToIntegral_ConversionsWork)
                {
                    any a;

                    a = 10.0f;
                    EXPECT_ANY_IS(&a, float, 10.0f);

                    EXPECT_ANY_IS(&a, int, 10);
                    EXPECT_ANY_IS(&a, long, 10);
                    EXPECT_ANY_IS(&a, char, 10);
                    EXPECT_ANY_IS(&a, unsigned int, 10u);
                    EXPECT_ANY_IS(&a, unsigned long, 10u);
                    EXPECT_ANY_IS(&a, unsigned char, 10u);
                }

                TEST_F(AnyTest, AnyNumericCast_FloatingPointToIntegral_AssertsOnDataLoss)
                {
                    any a;

                    // Test assert on out of range
                    a = DBL_MAX;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    float f;
                    any_numeric_cast(&a, f);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

#undef EXPECT_ANY_IS
            }
        }
    }
}

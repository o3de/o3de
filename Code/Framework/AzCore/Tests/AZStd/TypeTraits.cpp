/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"
#include <AzCore/std/typetraits/internal/is_template_copy_constructible.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    /**
     * Tests the AZSTD type traits as much as possible for each target.
     * \cond
     * The following templates require compiler support for a fully conforming implementation:
     * template <class T> struct is_class;
     * template <class T> struct is_union;
     * template <class T> struct is_enum;
     * template <class T> struct is_polymorphic;
     * template <class T> struct is_empty;
     * template <class T> struct has_trivial_constructor;
     * template <class T> struct has_trivial_copy;
     * template <class T> struct has_trivial_assign;
     * template <class T> struct has_trivial_destructor;
     * template <class T> struct has_nothrow_constructor;
     * template <class T> struct has_nothrow_copy;
     * template <class T> struct has_nothrow_assign;
     * template <class T> struct is_pod;
     * template <class T> struct is_abstract;
     * \endcond
     */
    TEST(TypeTraits, All)
    {
        //////////////////////////////////////////////////////////////////////////
        // Primary type categories:

        // alignment_of and align_to
        AZ_TEST_STATIC_ASSERT(alignment_of<int>::value == 4);
        AZ_TEST_STATIC_ASSERT(alignment_of<char>::value == 1);

        AZ_TEST_STATIC_ASSERT(alignment_of<MyClass>::value == 16);
        aligned_storage<sizeof(int)*100, 16>::type alignedArray;
        AZ_TEST_ASSERT((((AZStd::size_t)&alignedArray) & 15) == 0);

        AZ_TEST_STATIC_ASSERT((alignment_of< aligned_storage<sizeof(int)*5, 8>::type >::value) == 8);
        AZ_TEST_STATIC_ASSERT(sizeof(aligned_storage<sizeof(int), 16>::type) == 16);

        // is_void
        AZ_TEST_STATIC_ASSERT(is_void<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_void<void>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_void<void const volatile>::value == true);

        // is_integral
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned char>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned short>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<unsigned long>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed char>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed short>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<signed long>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<bool>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char>::value == true);
        //AZ_TEST_STATIC_ASSERT(is_integral<wchar_t>::value == true);

        AZ_TEST_STATIC_ASSERT(is_integral<char const >::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<char const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<short const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<int const volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_integral<long const volatile>::value == true);

        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct const>::value == false);
        AZ_TEST_STATIC_ASSERT(is_integral<MyStruct const volatile>::value == false);

        // is_floating_point
        AZ_TEST_STATIC_ASSERT(is_floating_point<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float const>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float volatile>::value == true);
        AZ_TEST_STATIC_ASSERT(is_floating_point<float const volatile>::value == true);

        // is_array
        AZ_TEST_STATIC_ASSERT(is_array<int*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_array<int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<volatile int[5]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const volatile int[5]>::value == true);

        AZ_TEST_STATIC_ASSERT(is_array<float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<volatile float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_array<const volatile float[]>::value == true);

        // is_pointer
        AZ_TEST_STATIC_ASSERT(is_pointer<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_pointer<int*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<const MyStruct*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<volatile int*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pointer<const volatile MyStruct*>::value == true);

        // is_reference
        AZ_TEST_STATIC_ASSERT(is_reference<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_reference<int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<const MyStruct&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<volatile int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_reference<const volatile MyStruct&>::value == true);

        // is_member_object_pointer
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<int (MyStruct::*)()>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_object_pointer<int MyStruct::*>::value == true);

        // is_member_function_pointer
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<int (MyStruct::*)()>::value == true);
        AZ_TEST_STATIC_ASSERT(is_member_function_pointer<int MyStruct::*>::value == false);
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() volatile>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() &>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() &&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const&&>::value));
        AZ_TEST_STATIC_ASSERT((is_member_function_pointer<int (MyStruct::*)() const volatile&&>::value));

        // is_enum
        AZ_TEST_STATIC_ASSERT(is_enum<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_enum<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_enum<MyEnum>::value == true);

        // is_union
        AZ_TEST_STATIC_ASSERT(is_union<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_union<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_union<MyUnion>::value == true);

        // is_class
        AZ_TEST_STATIC_ASSERT(is_class<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_class<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_class<MyClass>::value == true);

        // is_function
        AZ_TEST_STATIC_ASSERT(is_function<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_function<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_function<int(float, char)>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // composite type categories:

        // is_arithmetic
        AZ_TEST_STATIC_ASSERT(is_arithmetic<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_arithmetic<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_arithmetic<float>::value == true);

        // is_fundamental
        AZ_TEST_STATIC_ASSERT(is_fundamental<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_fundamental<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_fundamental<const float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_fundamental<void>::value == true);

        // is_object
        AZ_TEST_STATIC_ASSERT(is_object<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_object<MyStruct&>::value == false);
        AZ_TEST_STATIC_ASSERT(is_object<int(short, float)>::value == false);
        AZ_TEST_STATIC_ASSERT(is_object<void>::value == false);

        // is_scalar
        AZ_TEST_STATIC_ASSERT(is_scalar<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_scalar<MyStruct*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_scalar<const float>::value == true);
        AZ_TEST_STATIC_ASSERT(is_scalar<int>::value == true);

        // is_compound
        AZ_TEST_STATIC_ASSERT(is_compound<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_compound<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<int(short, float)>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<float[]>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<int&>::value == true);
        AZ_TEST_STATIC_ASSERT(is_compound<const void*>::value == true);

        // is_member_pointer
        AZ_TEST_STATIC_ASSERT(is_member_pointer<MyStruct*>::value == false);
        AZ_TEST_STATIC_ASSERT(is_member_pointer<int MyStruct::*>::value == true);
        AZ_TEST_STATIC_ASSERT(is_member_pointer<int (MyStruct::*)()>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // type properties:

        // is_const
        AZ_TEST_STATIC_ASSERT(is_const<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_const<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_const<const MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_const<const float>::value == true);

        // is_volatile
        AZ_TEST_STATIC_ASSERT(is_volatile<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_volatile<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_volatile<volatile MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_volatile<volatile float>::value == true);

        // is_pod
        AZ_TEST_STATIC_ASSERT(is_pod<MyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pod<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_pod<const MyClass>::value == false);
        AZ_TEST_STATIC_ASSERT((is_pod< aligned_storage<30, 32>::type >::value) == true);

        // is_empty
        AZ_TEST_STATIC_ASSERT(is_empty<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_empty<MyEmptyStruct>::value == true);
        AZ_TEST_STATIC_ASSERT(is_empty<int>::value == false);

        // is_polymorphic
        AZ_TEST_STATIC_ASSERT(is_polymorphic<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_polymorphic<MyClass>::value == true);

        // is_abstract
        AZ_TEST_STATIC_ASSERT(is_abstract<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_abstract<MyInterface>::value == true);

        // has_trivial_constructor
        static_assert(is_trivially_constructible_v<MyStruct>);
        static_assert(is_trivially_constructible_v<int>);
        static_assert(!is_trivially_constructible_v<MyClass>);

        // has_trivial_copy
        static_assert(is_trivially_copy_constructible_v<MyStruct>);
        static_assert(is_trivially_copy_constructible_v<int>);
        static_assert(!is_trivially_copy_constructible_v<MyClass>);

        // has_trivial_assign
        static_assert(is_trivially_copy_assignable_v<MyStruct>);
        static_assert(is_trivially_copy_assignable_v<int>);
        static_assert(!is_trivially_copy_assignable_v<MyClass>);

        // has_trivial_destructor
        static_assert(is_trivially_destructible_v<MyStruct>);
        static_assert(is_trivially_destructible_v<int>);
        static_assert(!is_trivially_destructible_v<MyClass>);

        // has_nothrow_constructr

        // has_nothrow_copy

        // has_nothrow_assign

        // is_signed
        AZ_TEST_STATIC_ASSERT(is_signed<int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_signed<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_signed<unsigned int>::value == false);
        static_assert(is_signed<float>::value);

        // is_unsigned
        AZ_TEST_STATIC_ASSERT(is_unsigned<int>::value == false);
        AZ_TEST_STATIC_ASSERT(is_unsigned<MyStruct>::value == false);
        AZ_TEST_STATIC_ASSERT(is_unsigned<unsigned int>::value == true);
        AZ_TEST_STATIC_ASSERT(is_unsigned<float>::value == false);

        // true and false types
        AZ_TEST_STATIC_ASSERT(true_type::value == true);
        AZ_TEST_STATIC_ASSERT(false_type::value == false);

        //! function traits tests
        struct NotMyStruct
        {
            int foo() { return 0; }

        };

        struct FunctionTestStruct
        {
            bool operator()(FunctionTestStruct&) const { return true; };
        };
        
        using PrimitiveFunctionPtr = int(*)(bool, float, double, AZ::u8, AZ::s8, AZ::u16, AZ::s16, AZ::u32, AZ::s32, AZ::u64, AZ::s64);
        using NotMyStructMemberPtr = int(NotMyStruct::*)();
        using ComplexFunctionPtr = float(*)(MyEmptyStruct&, NotMyStructMemberPtr, MyUnion*);
        using ComplexFunction = remove_pointer_t<ComplexFunctionPtr>;
        using MemberFunctionPtr = void(MyInterface::*)(int);
        using ConstMemberFunctionPtr = bool(FunctionTestStruct::*)(FunctionTestStruct&) const;

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::result_type, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::get_arg_t<10>, AZ::s64>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_arg_t<PrimitiveFunctionPtr, 5>, AZ::u16>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<PrimitiveFunctionPtr>::arity == 11));
        
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_result_t<ComplexFunction>, float>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<AZStd::function_traits_get_arg_t<ComplexFunction, 1>, int(NotMyStruct::*)()>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<ComplexFunction>::arity == 3));

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_fp_type, void(MyInterface::*)(int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::raw_fp_type, void(*)(int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_type, MyInterface>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<MemberFunctionPtr>::arity == 1));

        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const> ::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::raw_fp_type, bool(*)(FunctionTestStruct&)> ::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_type, FunctionTestStruct>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits_get_arg_t<ConstMemberFunctionPtr, 0>, FunctionTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<ConstMemberFunctionPtr>::arity == 1));
        
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<decltype(&FunctionTestStruct::operator())>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const>::value));

        auto lambdaFunction = [](FunctionTestStruct, int) -> bool
        {
            return false;
        };

        using LambdaType = decltype(lambdaFunction);
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<LambdaType>::raw_fp_type, bool(*)(FunctionTestStruct, int)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<LambdaType>::class_fp_type, bool(LambdaType::*)(FunctionTestStruct, int) const>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<LambdaType>::arity == 2));
        static_assert(AZStd::is_same<AZStd::function_traits<LambdaType>::return_type, bool>::value, "Lambda result type should be bool");

        AZStd::function<void(LambdaType*, ComplexFunction&)> stdFunction;
        using StdFunctionType = decay_t<decltype(stdFunction)>;
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<typename AZStd::function_traits<StdFunctionType>::raw_fp_type, void(*)(LambdaType*, ComplexFunction&)>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::function_traits<StdFunctionType>::arity == 2));
    } 

    struct ConstMethodTestStruct
    {
        void ConstMethod() const { }
        void NonConstMethod() { }
    };

    AZ_TEST_STATIC_ASSERT((static_cast<uint32_t>(function_traits<decltype(&ConstMethodTestStruct::ConstMethod)>::qual_flags) & static_cast<uint32_t>(Internal::qualifier_flags::const_)) != 0);
    AZ_TEST_STATIC_ASSERT((static_cast<uint32_t>(function_traits<decltype(&ConstMethodTestStruct::NonConstMethod)>::qual_flags) & static_cast<uint32_t>(Internal::qualifier_flags::const_)) == 0);
}

TEST(TypeTraits, StdRemoveConstCompiles)
{
    static_assert(AZStd::is_same_v<int, AZStd::remove_const_t<const int>>, "C++11 std::remove_const_t has failed");
    static_assert(AZStd::is_same_v<int, AZStd::remove_const_t<int>>, "C++11 std::remove_const_t has failed");
    static_assert(AZStd::is_same_v<int*, AZStd::remove_const_t<int* const>>, "C++11 std::remove_const_t has failed");
    static_assert(AZStd::is_same_v<const int*, AZStd::remove_const_t<const int*>>, "C++11 std::remove_const_t has failed");
    static_assert(AZStd::is_same_v<const volatile int*, AZStd::remove_const_t<const volatile int* const>>, "C++11 std::remove_const_t has failed");
    static_assert(AZStd::is_same_v<int, AZStd::remove_const_t<AZStd::remove_reference_t<const int&>>>, "C++11 std::remove_const_t has failed");
}

TEST(TypeTraits, StdRemoveVolatileCompiles)
{
    static_assert(AZStd::is_same_v<int, AZStd::remove_volatile_t<volatile int>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<int, AZStd::remove_volatile_t<int>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<int*, AZStd::remove_volatile_t<int* volatile>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<volatile int*, AZStd::remove_volatile_t<volatile int*>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<const volatile int*, AZStd::remove_volatile_t<const volatile int*>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<const int*, AZStd::remove_volatile_t<const int* volatile>>, "C++11 std::remove_volatile_t has failed");
    static_assert(AZStd::is_same_v<int, AZStd::remove_volatile_t<AZStd::remove_reference_t<volatile int&>>>, "C++11 std::remove_volatile_t has failed");
}

TEST(TypeTraits, StdIsConstCompiles)
{
    static_assert(!AZStd::is_const_v<int>, "C++11 std::is_const has failed");
    static_assert(AZStd::is_const_v<const int>, "C++11 std::is_const has failed");
    // references are never const
    static_assert(!AZStd::is_const_v<const int&>, "C++11 std::is_const has failed");
    // pointer checks for constness
    static_assert(!AZStd::is_const_v<const int*>, "C++11 std::is_const has failed");
    static_assert(AZStd::is_const_v<const int* const>, "C++11 std::is_const has failed");
    static_assert(AZStd::is_const_v<int* const>, "C++11 std::is_const has failed");
}

TEST(TypeTraits, StdIsVolatileCompiles)
{
    static_assert(!AZStd::is_volatile_v<int>, "C++11 std::is_volatile has failed");
    static_assert(AZStd::is_volatile_v<volatile int>, "C++11 std::is_volatile has failed");
    // references are never volatile
    static_assert(!AZStd::is_volatile_v<volatile int&>, "C++11 std::is_volatile has failed");
    // pointer checks for volatile
    static_assert(!AZStd::is_volatile_v<volatile int*>, "C++11 std::is_volatile has failed");
    static_assert(AZStd::is_volatile_v<const int* volatile>, "C++11 std::is_volatile has failed");
    static_assert(!AZStd::is_volatile_v<volatile int* const>, "C++11 std::is_volatile has failed");
    static_assert(AZStd::is_volatile_v<int* volatile>, "C++11 std::is_volatile has failed");
}

TEST(TypeTraits, TemplateIsCopyConstructible_WithCopyConstructibleValueType_ReturnsTrue)
{
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::vector<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::list<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::forward_list<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::map<int, int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::multimap<int, int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::unordered_map<int, int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::unordered_multimap<int, int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::set<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::multiset<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::unordered_set<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::unordered_multiset<int>>::value, "");
    static_assert(AZStd::Internal::template_is_copy_constructible<AZStd::pair<int, int>>::value, "");

    struct CopyableType
    {
        CopyableType() = default;
        CopyableType(const CopyableType&) = default;
    };
    static_assert(AZStd::Internal::template_is_copy_constructible<CopyableType>::value, "");
}

TEST(TypeTraits, TemplateIsCopyConstructible_WithOutCopyConstructibleValueType_ReturnsFalse)
{
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::vector<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::list<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::forward_list<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::map<AZStd::unique_ptr<int>, int>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::map<int, AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::multimap<AZStd::unique_ptr<int>, int>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::multimap<int, AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_map<AZStd::unique_ptr<int>, int>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_map<int, AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_multimap<AZStd::unique_ptr<int>, int>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_multimap<int, AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::set<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::multiset<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_set<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::unordered_multiset<AZStd::unique_ptr<int>>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::pair<AZStd::unique_ptr<int>, int>>::value, "");
    static_assert(!AZStd::Internal::template_is_copy_constructible<AZStd::pair<int, AZStd::unique_ptr<int>>>::value, "");

    struct MoveOnly
    {
        MoveOnly() = default;
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
    };
    static_assert(!AZStd::Internal::template_is_copy_constructible<MoveOnly>::value, "");
}

TEST(TypeTraits, MakeSignedCompiles)
{
    static_assert(AZStd::is_same_v<typename AZStd::make_signed<AZ::s8>::type, AZ::s8>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::u8>, AZ::s8>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::s16>, AZ::s16>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::u16>, AZ::s16>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::s32>, AZ::s32>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::u32>, AZ::s32>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::s64>, AZ::s64>);
    static_assert(AZStd::is_same_v<AZStd::make_signed_t<AZ::s64>, AZ::s64>);
}

TEST(TypeTraits, MakeUnsignedCompiles)
{
    static_assert(AZStd::is_same_v<typename AZStd::make_unsigned<AZ::s8>::type, AZ::u8>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::u8>, AZ::u8>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::s16>, AZ::u16>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::u16>, AZ::u16>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::s32>, AZ::u32>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::u32>, AZ::u32>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::s64>, AZ::u64>);
    static_assert(AZStd::is_same_v<AZStd::make_unsigned_t<AZ::s64>, AZ::u64>);
}

// VS2017 workaround, calling decltype directly on the fully specialized aznumeric_cast template
// function fails with error C3556:  'aznumeric_cast': incorrect argument to 'decltype'
// So invoke the attempt to invoke function in a non-evaluated context and SFINAE to prevent a compile
// error
template <typename T, typename = void>
constexpr bool NumericCastInvocable = false;
template <typename T>
constexpr bool NumericCastInvocable<T, AZStd::void_t<decltype(aznumeric_cast<int>(AZStd::declval<T>()))>> = true;
TEST(TypeTraits, NumericCastConversionOperatorCompiles)
{
    struct AzNumericCastConvertibleCompileTest
    {
        constexpr operator int() { return {}; };
    };
    static_assert(NumericCastInvocable<AzNumericCastConvertibleCompileTest>, "aznumeric_cast conversion operator overload is should be compilable");
}

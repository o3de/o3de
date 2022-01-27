/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"
#include <AzCore/std/typetraits/internal/is_template_copy_constructible.h>
#include <AzCore/std/containers/forward_list.h>
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
        static_assert(alignment_of<int>::value == 4);
        static_assert(alignment_of<char>::value == 1);

        static_assert(alignment_of<MyClass>::value == 16);
        aligned_storage<sizeof(int) * 100, 16>::type alignedArray;
        AZ_TEST_ASSERT((((AZStd::size_t)&alignedArray) & 15) == 0);

        static_assert((alignment_of< aligned_storage<sizeof(int) * 5, 8>::type >::value) == 8);
        static_assert(sizeof(aligned_storage<sizeof(int), 16>::type) == 16);

        // is_void
        static_assert(is_void<int>::value == false);
        static_assert(is_void<void>::value == true);
        static_assert(is_void<void const>::value == true);
        static_assert(is_void<void volatile>::value == true);
        static_assert(is_void<void const volatile>::value == true);

        // is_integral
        static_assert(is_integral<unsigned char>::value == true);
        static_assert(is_integral<unsigned short>::value == true);
        static_assert(is_integral<unsigned int>::value == true);
        static_assert(is_integral<unsigned long>::value == true);
        static_assert(is_integral<signed char>::value == true);
        static_assert(is_integral<signed short>::value == true);
        static_assert(is_integral<signed int>::value == true);
        static_assert(is_integral<signed long>::value == true);
        static_assert(is_integral<bool>::value == true);
        static_assert(is_integral<char>::value == true);
        //static_assert(is_integral<wchar_t>::value == true);

        static_assert(is_integral<char const >::value == true);
        static_assert(is_integral<short const>::value == true);
        static_assert(is_integral<int const>::value == true);
        static_assert(is_integral<long const>::value == true);
        static_assert(is_integral<char volatile>::value == true);
        static_assert(is_integral<short volatile>::value == true);
        static_assert(is_integral<int volatile>::value == true);
        static_assert(is_integral<long volatile>::value == true);
        static_assert(is_integral<char const volatile>::value == true);
        static_assert(is_integral<short const volatile>::value == true);
        static_assert(is_integral<int const volatile>::value == true);
        static_assert(is_integral<long const volatile>::value == true);

        static_assert(is_integral<MyStruct>::value == false);
        static_assert(is_integral<MyStruct const>::value == false);
        static_assert(is_integral<MyStruct const volatile>::value == false);

        // is_floating_point
        static_assert(is_floating_point<int>::value == false);
        static_assert(is_floating_point<float>::value == true);
        static_assert(is_floating_point<float const>::value == true);
        static_assert(is_floating_point<float volatile>::value == true);
        static_assert(is_floating_point<float const volatile>::value == true);

        // is_array
        static_assert(is_array<int*>::value == false);
        static_assert(is_array<int[5]>::value == true);
        static_assert(is_array<const int[5]>::value == true);
        static_assert(is_array<volatile int[5]>::value == true);
        static_assert(is_array<const volatile int[5]>::value == true);

        static_assert(is_array<float[]>::value == true);
        static_assert(is_array<const float[]>::value == true);
        static_assert(is_array<volatile float[]>::value == true);
        static_assert(is_array<const volatile float[]>::value == true);

        // is_pointer
        static_assert(is_pointer<int>::value == false);
        static_assert(is_pointer<int*>::value == true);
        static_assert(is_pointer<const MyStruct*>::value == true);
        static_assert(is_pointer<volatile int*>::value == true);
        static_assert(is_pointer<const volatile MyStruct*>::value == true);

        // is_reference
        static_assert(is_reference<int>::value == false);
        static_assert(is_reference<int&>::value == true);
        static_assert(is_reference<const MyStruct&>::value == true);
        static_assert(is_reference<volatile int&>::value == true);
        static_assert(is_reference<const volatile MyStruct&>::value == true);

        // is_member_object_pointer
        static_assert(is_member_object_pointer<MyStruct*>::value == false);
        static_assert(is_member_object_pointer<int (MyStruct::*)()>::value == false);
        static_assert(is_member_object_pointer<int MyStruct::*>::value == true);

        // is_member_function_pointer
        static_assert(is_member_function_pointer<MyStruct*>::value == false);
        static_assert(is_member_function_pointer<int (MyStruct::*)()>::value == true);
        static_assert(is_member_function_pointer<int MyStruct::*>::value == false);
        static_assert((is_member_function_pointer<int (MyStruct::*)() const>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() volatile>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() const volatile>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)()&>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() const&>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() const volatile&>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)()&&>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() const&&>::value));
        static_assert((is_member_function_pointer<int (MyStruct::*)() const volatile&&>::value));

        // is_enum
        static_assert(is_enum<int>::value == false);
        static_assert(is_enum<MyStruct>::value == false);
        static_assert(is_enum<MyEnum>::value == true);

        // is_union
        static_assert(is_union<int>::value == false);
        static_assert(is_union<MyStruct>::value == false);
        static_assert(is_union<MyUnion>::value == true);

        // is_class
        static_assert(is_class<int>::value == false);
        static_assert(is_class<MyStruct>::value == true);
        static_assert(is_class<MyClass>::value == true);

        // is_function
        static_assert(is_function<int>::value == false);
        static_assert(is_function<MyStruct>::value == false);
        static_assert(is_function<int(float, char)>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // composite type categories:

        // is_arithmetic
        static_assert(is_arithmetic<MyStruct>::value == false);
        static_assert(is_arithmetic<int>::value == true);
        static_assert(is_arithmetic<float>::value == true);

        // is_fundamental
        static_assert(is_fundamental<MyStruct>::value == false);
        static_assert(is_fundamental<int>::value == true);
        static_assert(is_fundamental<const float>::value == true);
        static_assert(is_fundamental<void>::value == true);

        // is_object
        static_assert(is_object<MyStruct>::value == true);
        static_assert(is_object<MyStruct&>::value == false);
        static_assert(is_object<int(short, float)>::value == false);
        static_assert(is_object<void>::value == false);

        // is_scalar
        static_assert(is_scalar<MyStruct>::value == false);
        static_assert(is_scalar<MyStruct*>::value == true);
        static_assert(is_scalar<const float>::value == true);
        static_assert(is_scalar<int>::value == true);

        // is_compound
        static_assert(is_compound<int>::value == false);
        static_assert(is_compound<MyStruct>::value == true);
        static_assert(is_compound<int(short, float)>::value == true);
        static_assert(is_compound<float[]>::value == true);
        static_assert(is_compound<int&>::value == true);
        static_assert(is_compound<const void*>::value == true);

        // is_member_pointer
        static_assert(is_member_pointer<MyStruct*>::value == false);
        static_assert(is_member_pointer<int MyStruct::*>::value == true);
        static_assert(is_member_pointer<int (MyStruct::*)()>::value == true);

        //////////////////////////////////////////////////////////////////////////
        // type properties:

        // is_const
        static_assert(is_const<MyStruct>::value == false);
        static_assert(is_const<int>::value == false);
        static_assert(is_const<const MyStruct>::value == true);
        static_assert(is_const<const float>::value == true);

        // is_volatile
        static_assert(is_volatile<MyStruct>::value == false);
        static_assert(is_volatile<int>::value == false);
        static_assert(is_volatile<volatile MyStruct>::value == true);
        static_assert(is_volatile<volatile float>::value == true);

        // is_pod
        static_assert(is_pod<MyStruct>::value == true);
        static_assert(is_pod<int>::value == true);
        static_assert(is_pod<const MyClass>::value == false);
        static_assert((is_pod< aligned_storage<30, 32>::type >::value) == true);

        // is_empty
        static_assert(is_empty<MyStruct>::value == false);
        static_assert(is_empty<MyEmptyStruct>::value == true);
        static_assert(is_empty<int>::value == false);

        // is_polymorphic
        static_assert(is_polymorphic<MyStruct>::value == false);
        static_assert(is_polymorphic<MyClass>::value == true);

        // is_abstract
        static_assert(is_abstract<MyStruct>::value == false);
        static_assert(is_abstract<MyInterface>::value == true);

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
        static_assert(is_signed<int>::value == true);
        static_assert(is_signed<MyStruct>::value == false);
        static_assert(is_signed<unsigned int>::value == false);
        static_assert(is_signed<float>::value);

        // is_unsigned
        static_assert(is_unsigned<int>::value == false);
        static_assert(is_unsigned<MyStruct>::value == false);
        static_assert(is_unsigned<unsigned int>::value == true);
        static_assert(is_unsigned<float>::value == false);

        // true and false types
        static_assert(true_type::value == true);
        static_assert(false_type::value == false);

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

        static_assert((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::result_type, int>::value));
        static_assert((AZStd::is_same<AZStd::function_traits<PrimitiveFunctionPtr>::get_arg_t<10>, AZ::s64>::value));
        static_assert((AZStd::is_same<AZStd::function_traits_get_arg_t<PrimitiveFunctionPtr, 5>, AZ::u16>::value));
        static_assert((AZStd::function_traits<PrimitiveFunctionPtr>::arity == 11));

        static_assert((AZStd::is_same<AZStd::function_traits_get_result_t<ComplexFunction>, float>::value));
        static_assert((AZStd::is_same<AZStd::function_traits_get_arg_t<ComplexFunction, 1>, int(NotMyStruct::*)()>::value));
        static_assert((AZStd::function_traits<ComplexFunction>::arity == 3));

        static_assert((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_fp_type, void(MyInterface::*)(int)>::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::raw_fp_type, void(*)(int)>::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits<MemberFunctionPtr>::class_type, MyInterface>::value));
        static_assert((AZStd::function_traits<MemberFunctionPtr>::arity == 1));

        static_assert((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const> ::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::raw_fp_type, bool(*)(FunctionTestStruct&)> ::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits<ConstMemberFunctionPtr>::class_type, FunctionTestStruct>::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits_get_arg_t<ConstMemberFunctionPtr, 0>, FunctionTestStruct&>::value));
        static_assert((AZStd::function_traits<ConstMemberFunctionPtr>::arity == 1));

        static_assert((AZStd::is_same<typename AZStd::function_traits<decltype(&FunctionTestStruct::operator())>::class_fp_type, bool(FunctionTestStruct::*)(FunctionTestStruct&) const>::value));

        auto lambdaFunction = [](FunctionTestStruct, int) -> bool
        {
            return false;
        };

        using LambdaType = decltype(lambdaFunction);
        static_assert((AZStd::is_same<typename AZStd::function_traits<LambdaType>::raw_fp_type, bool(*)(FunctionTestStruct, int)>::value));
        static_assert((AZStd::is_same<typename AZStd::function_traits<LambdaType>::class_fp_type, bool(LambdaType::*)(FunctionTestStruct, int) const>::value));
        static_assert((AZStd::function_traits<LambdaType>::arity == 2));
        static_assert(AZStd::is_same<AZStd::function_traits<LambdaType>::return_type, bool>::value, "Lambda result type should be bool");

        AZStd::function<void(LambdaType*, ComplexFunction&)> stdFunction;
        using StdFunctionType = decay_t<decltype(stdFunction)>;
        static_assert((AZStd::is_same<typename AZStd::function_traits<StdFunctionType>::raw_fp_type, void(*)(LambdaType*, ComplexFunction&)>::value));
        static_assert((AZStd::function_traits<StdFunctionType>::arity == 2));
    }

    struct ConstMethodTestStruct
    {
        void ConstMethod() const { }
        void NonConstMethod() { }
    };

    static_assert((static_cast<uint32_t>(function_traits<decltype(&ConstMethodTestStruct::ConstMethod)>::qual_flags)& static_cast<uint32_t>(Internal::qualifier_flags::const_)) != 0);
    static_assert((static_cast<uint32_t>(function_traits<decltype(&ConstMethodTestStruct::NonConstMethod)>::qual_flags)& static_cast<uint32_t>(Internal::qualifier_flags::const_)) == 0);

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


    struct CommonReferenceSpecializationTest
    {};
}

namespace AZStd
{
    template <template <class> class TQual, template <class> class UQual>
    struct basic_common_reference<UnitTest::CommonReferenceSpecializationTest, int, TQual, UQual>
    {
        using type = int&;
    };
    template <template <class> class TQual, template <class> class UQual>
    struct basic_common_reference<int, UnitTest::CommonReferenceSpecializationTest, TQual, UQual>
    {
        using type = int&;
    };

}

namespace UnitTest
{
    TEST(TypeTraits, StdCommonReferenceCompiles)
    {
        // Test std::common_reference bullet https://eel.is/c++draft/meta.trans.other#6.3.1
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<volatile float&, const float&>, const volatile float&>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<float&&, const float&>, const float&>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<const float&&, const float&>, const float&>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<int&&, float&>, float>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<int, float&&>, float>, "C++20 std::common_reference has failed");
        // Test std::common_reference customization point https://eel.is/c++draft/meta.trans.other#6.3.2
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<UnitTest::CommonReferenceSpecializationTest, int>, int&>, "C++ basic_common_reference_customization_point has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<int, UnitTest::CommonReferenceSpecializationTest>, int&>, "C++ basic_common_reference_customization_point has failed");
        // Test std::common_reference bullet https://eel.is/c++draft/meta.trans.other#6.3.3
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<int, float>, float>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<float, int>, float>, "C++20 std::common_reference has failed");
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<float, const int>, float>, "C++20 std::common_reference has failed");
        // Test std::common_reference bullet https://eel.is/c++draft/meta.trans.other#6.3.3
        using Int64Milliseconds = AZStd::chrono::duration<int64_t, AZStd::milli>;
        using DoubleMilliseconds = AZStd::chrono::duration<double, AZStd::milli>;
        static_assert(AZStd::is_same_v<AZStd::common_reference_t<Int64Milliseconds, DoubleMilliseconds>, DoubleMilliseconds>,
            "C++20 std::common_reference common_type check has failed");
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

    struct IteratorTraitsSpecializationTest {};
}

namespace AZStd
{
    template <>
    struct iterator_traits<UnitTest::IteratorTraitsSpecializationTest>
    {
        using difference_type = ptrdiff_t;
        using value_type = char;
        using pointer = char*;
        using reference = char&;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = contiguous_iterator_tag;
    };
}

namespace UnitTest
{
    TEST(TypeTraits, IterValueCompiles)
    {
        struct IterValueWithValueAndElementTypeTest
        {
            using element_type = volatile char;
            using value_type = const char;
        };

        static_assert(AZStd::is_same_v<AZStd::iter_value_t<AZStd::string_view>, typename AZStd::string_view::value_type>);
        static_assert(AZStd::is_same_v<AZStd::iter_value_t<AZStd::unique_ptr<char>>, typename AZStd::unique_ptr<char>::element_type>);
        static_assert(AZStd::is_same_v<AZStd::iter_value_t<AZ::u32[]>, AZ::u32>);
        static_assert(AZStd::is_same_v<AZStd::iter_value_t<AZ::u32*>, AZ::u32>);
        static_assert(AZStd::is_same_v<AZStd::iter_value_t<IterValueWithValueAndElementTypeTest>, char>);
        static_assert(AZStd::is_same_v<AZStd::iter_value_t<UnitTest::IteratorTraitsSpecializationTest>, char>);
    }
    TEST(TypeTraits, IterDifferenceCompiles)
    {
        struct IterDifferenceTest
        {
            using difference_type = AZ::s8;
        };

        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<AZ::s32>, AZ::s32>);
        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<AZ::u32>, AZ::s32 >);
        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<AZ::s32*>, ptrdiff_t>);
        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<IterDifferenceTest>, AZ::s8>);
        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<const IterDifferenceTest>, AZ::s8>);
        static_assert(AZStd::is_same_v<AZStd::iter_difference_t<UnitTest::IteratorTraitsSpecializationTest>, ptrdiff_t>);
    }
    TEST(TypeTraits, IterReferenceCompiles)
    {
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<typename AZStd::string_view::iterator>, const char&>);
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<typename AZStd::list<char>::iterator>, char&>);
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<AZ::u32[]>, AZ::u32&>);
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<AZ::u32*>, AZ::u32&>);
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<typename AZStd::list<char>::const_iterator>, const char&>);
        static_assert(AZStd::is_same_v<AZStd::iter_reference_t<std::istreambuf_iterator<char>>, char>);
    }

    // Test customizing the ranges::iter_move function
    struct IterMoveCustomizationPointTest
    {
        int operator*();
    };
    char&& iter_move(IterMoveCustomizationPointTest&);
    TEST(TypeTraits, IterRvalueReferenceCompiles)
    {
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<typename AZStd::string_view::iterator>, const char&&>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<typename AZStd::list<char>::iterator>, char&&>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<AZ::u32[]>, AZ::u32&&>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<AZ::u32*>, AZ::u32&&>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<typename AZStd::list<char>::const_iterator>, const char&&>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<std::istreambuf_iterator<char>>, char>);
        static_assert(AZStd::is_same_v<AZStd::iter_rvalue_reference_t<IterMoveCustomizationPointTest>, char&&>);
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
        static_assert(NumericCastInvocable<AzNumericCastConvertibleCompileTest>, "aznumeric_cast conversion operator overload should be compilable");
    }
}

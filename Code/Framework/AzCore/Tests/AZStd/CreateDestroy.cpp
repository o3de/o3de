/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>
#include <array>

using namespace UnitTestInternal;

namespace UnitTest
{
    TEST(CreateDestroy, UninitializedFill_StdArray_IntType_AllEight)
    {
        const int intArraySize = 5;
        const int fillValue = 8;
        std::array<int, intArraySize> intArray;
        AZStd::uninitialized_fill(intArray.begin(), intArray.end(), fillValue, std::false_type());
        for (auto itr : intArray)
        {
            EXPECT_EQ(itr, fillValue);
        }
    }

    TEST(CreateDestroy, UninitializedFill_AZStdArray_IntType_AllEight)
    {
        const int intArraySize = 5;
        const int fillValue = 8;
        AZStd::array<int, intArraySize> intArray;
        AZStd::uninitialized_fill(intArray.begin(), intArray.end(), fillValue, std::false_type());
        for (auto itr : intArray)
        {
            EXPECT_EQ(itr, fillValue);
        }
    }

    TEST(CreateDestroy, UninitializedFill_StdArray_StringType_AllEight)
    {
        const int stringArraySize = 5;
        const AZStd::string fillValue = "hello, world";
        std::array<AZStd::string, stringArraySize> stringArray;
        AZStd::uninitialized_fill(stringArray.begin(), stringArray.end(), fillValue, std::false_type());
        for (auto itr : stringArray)
        {
            EXPECT_EQ(0, strcmp(itr.c_str(), fillValue.c_str()));
        }
    }

    TEST(CreateDestroy, UninitializedFill_AZStdArray_StringType_AllEight)
    {
        const int stringArraySize = 5;
        const AZStd::string fillValue = "hello, world";
        AZStd::array<AZStd::string, stringArraySize> stringArray;
        AZStd::uninitialized_fill(stringArray.begin(), stringArray.end(), fillValue, std::false_type());
        for (auto itr : stringArray)
        {
            EXPECT_EQ(0, strcmp(itr.c_str(), fillValue.c_str()));
        }
    }

    TEST(CreateDestroy, Destroy_Compile_WhenUsedInConstexpr)
    {   
        auto TestDestroyFunc = []() constexpr -> int
        {
            AZStd::string_view testValue("Test");
            AZStd::Internal::destroy<decltype(testValue)*>::single(&testValue);

            AZStd::string_view testArray[] = { AZStd::string_view("Test"), AZStd::string_view("World") };
            AZStd::Internal::destroy<decltype(testValue)*>::range(AZStd::begin(testArray), AZStd::end(testArray));
            return 73;
        };

        static_assert(AZStd::is_trivially_destructible_v<AZStd::string_view>);
        static_assert(TestDestroyFunc() == 73);
    }

    TEST(CreateDestroy, IsFastCopyTraits_SucceedForContiguousIteratorTypes)
    {
        using list_type = AZStd::list<int>;
        using vector_type = AZStd::vector<int>;
        using string_type = AZStd::string;
        static_assert(AZStd::Internal::is_fast_copy<const char*, const char*>::value);
        static_assert(!AZStd::Internal::is_fast_copy<typename list_type::iterator, int*>::value);
        static_assert(!AZStd::Internal::is_fast_copy<int*, typename list_type::iterator>::value);
        static_assert(AZStd::Internal::is_fast_copy<typename vector_type::iterator, int*>::value);
        static_assert(AZStd::Internal::is_fast_copy<int*, typename vector_type::iterator>::value);
        static_assert(AZStd::Internal::is_fast_copy<typename string_type::iterator, char*>::value);
        static_assert(AZStd::Internal::is_fast_copy<char*, typename string_type::iterator>::value);

        static_assert(!AZStd::Internal::is_fast_copy<typename vector_type::iterator, typename list_type::iterator>::value);
    }

    TEST(CreateDestroy, IsFastFillTraits_TrueForContiguousIteratorTypes)
    {
        using list_type = AZStd::list<int>;
        using vector_type = AZStd::vector<int>;
        using string_type = AZStd::string;
        using fixed_string_type = AZStd::fixed_string<128>;

        static_assert(!AZStd::Internal::is_fast_fill<typename list_type::iterator>::value);
        static_assert(!AZStd::Internal::is_fast_fill<typename vector_type::iterator>::value);
        // Fast fill requires the type to be of size 1
        static_assert(AZStd::Internal::is_fast_fill<const char*>::value);
        static_assert(AZStd::Internal::is_fast_fill<typename string_type::iterator>::value);
        static_assert(AZStd::Internal::is_fast_fill<typename fixed_string_type::iterator>::value);
    }

    TEST(CreateDestroy, ConstructAt_IsAbleToConstructReferenceTypes_Success)
    {
        struct TestConstructAt
        {
            TestConstructAt(int& intRef, float&& floatRef)
                : m_intRef(intRef)
                , m_floatValue(floatRef)
            {}

            int& m_intRef;
            float m_floatValue;
        };

        int testValue = 32;
        AZStd::aligned_storage_for_t<TestConstructAt> constructStorage;
        auto constructAddress = &reinterpret_cast<TestConstructAt&>(constructStorage);
        auto resultAddress = AZStd::construct_at(constructAddress, testValue, 4.0f);
        resultAddress->m_intRef = 22;

        EXPECT_EQ(22, testValue);
        EXPECT_FLOAT_EQ(4.0f, resultAddress->m_floatValue);
        AZStd::destroy_at(resultAddress);
    }
}

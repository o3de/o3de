/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Tests/Serialization/Json/JsonSerializationTests.h>

namespace JsonSerializationTests
{
    TEST_F(JsonSerializationTests, Compare_DifferentTypes_StringTypeLessThanNumberType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kStringType);
        rapidjson::Value rhs(rapidjson::kNumberType);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_NullType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kNullType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_TrueType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kTrueType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_FalseType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kFalseType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalStrings_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetString(rapidjson::StringRef("Hello"));
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentStrings_HelloLessThanWorld)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetString(rapidjson::StringRef("Hello"));
        rhs.SetString(rapidjson::StringRef("World"));
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalIntegers_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetUint64(42);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentIntegers_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetUint64(88);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalDoubles_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetDouble(42.0);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentDoubles_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetDouble(42.0);
        rhs.SetDouble(88.0);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalMixedNumbers_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetDouble(42.0);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(lhs, rhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMixedNumbers_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetDouble(88.0);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalObjects_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& value = m_jsonDocument->SetObject();
        value.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        value.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        value.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalObjectsWithDifferentOrder_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& lhs = m_jsonDocument->SetObject();
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value& rhs = m_jsonDocument->SetObject();
        rhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(lhs, rhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentSizedObjects_LessForSmallestObject)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMembersInObjects_AlphabeticallyLessBasedOnFieldNames)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("x"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("b"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("y"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("z"), true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMemberTypesObjects_LessBasedOnTheMemberType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), false, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), 88.0, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("c"), 88, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 42.0, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentValuesInObjectMembers_LessBasedOnMemberValue)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("c"), false, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 88, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalArray_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& value = m_jsonDocument->SetArray();
        value.PushBack(42, m_jsonDocument->GetAllocator());
        value.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        value.PushBack(true, m_jsonDocument->GetAllocator());
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentSizedArrays_LessForSmallestArray)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(42, m_jsonDocument->GetAllocator());
        lhs.PushBack(true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(42, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.PushBack(true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentTypesInArrays_LessBasedOnType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(false, m_jsonDocument->GetAllocator());
        lhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.PushBack(42, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(88.0, m_jsonDocument->GetAllocator());
        rhs.PushBack(false, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentValuesInArrays_LessBasedOnTheValue)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(42, m_jsonDocument->GetAllocator());
        lhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.PushBack(42.0, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(42, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());
        rhs.PushBack(88.0, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }
} // namespace JsonSerializationTests

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCoreTestingUtils;

using OBJECT_TYPE = TestObject<AZStd::string>;
using ARRAY_TYPE = AZStd::vector<AZStd::string>;
using ARRAY_OF_ARRAY_TYPE = AZStd::vector<AZStd::vector<AZStd::string>>;
using ARRAY_OF_OBJECT_TYPE = AZStd::vector<TestObject<AZStd::string>>;
using JsonReaderTest = UnitTest::ScopedAllocatorSetupFixture;

template<class ValueType>
void TestJsonReaderSuccess(const ValueType& expectedValue, const char* valueString)
{
    Aws::StringStream stringStream{TestObjectJson(valueString).c_str()};
    AWSCore::JsonInputStream jsonStream{stringStream};

    TestObject<ValueType> object;

    AZStd::string errorMessage;

    bool ok = AWSCore::JsonReader::ReadObject(jsonStream, object, errorMessage);

    ValueType& actualValue = object.value;

    ASSERT_TRUE(ok);
    ASSERT_TRUE(errorMessage.empty());
    ASSERT_EQ(actualValue, expectedValue);
}

template<class ValueType>
void TestJsonReaderFailure(const char* valueString)
{
    Aws::StringStream stringStream{TestObjectJson(valueString).c_str()};
    AWSCore::JsonInputStream jsonStream{stringStream};

    TestObject<ValueType> object;

    AZStd::string errorMessage;

    bool ok = AWSCore::JsonReader::ReadObject(jsonStream, object, errorMessage);

    ASSERT_FALSE(ok);
    ASSERT_FALSE(errorMessage.empty());

    puts(errorMessage.c_str());
}

TEST_F(JsonReaderTest, ReadObject_ReadAsStringType_GetExpectedResult)
{
    TestJsonReaderSuccess<AZStd::string>(STRING_VALUE, STRING_VALUE_JSON);
    TestJsonReaderFailure<AZStd::string>(INT_VALUE_STRING);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsBooleanType_GetExpectedResult)
{
    TestJsonReaderSuccess<bool>(BOOL_VALUE, BOOL_VALUE_STRING);
    TestJsonReaderFailure<bool>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsIntType_GetExpectedResult)
{
    TestJsonReaderSuccess<int>(INT_VALUE, INT_VALUE_STRING);
    TestJsonReaderSuccess<int>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonReaderFailure<int>(UINT_VALUE_MAX_STRING);
    TestJsonReaderFailure<int>(INT64_VALUE_STRING);
    TestJsonReaderFailure<int>(UINT64_VALUE_STRING);
    TestJsonReaderFailure<int>(DOUBLE_VALUE_STRING);
    TestJsonReaderFailure<int>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsUnsignedIntType_GetExpectedResult)
{
    TestJsonReaderFailure<unsigned>(INT_VALUE_STRING);
    TestJsonReaderSuccess<unsigned>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonReaderFailure<unsigned>(INT64_VALUE_STRING);
    TestJsonReaderFailure<unsigned>(UINT64_VALUE_STRING);
    TestJsonReaderFailure<unsigned>(DOUBLE_VALUE_STRING);
    TestJsonReaderFailure<unsigned>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsInt64Type_GetExpectedResult)
{
    TestJsonReaderSuccess<int64_t>(INT_VALUE, INT_VALUE_STRING);
    TestJsonReaderSuccess<int64_t>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonReaderSuccess<int64_t>(INT64_VALUE, INT64_VALUE_STRING);
    TestJsonReaderSuccess<int64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
    TestJsonReaderFailure<int64_t>(UINT64_VALUE_MAX_STRING);
    TestJsonReaderFailure<int64_t>(DOUBLE_VALUE_STRING);
    TestJsonReaderFailure<int64_t>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsUnsignedInt64Type_GetExpectedResult)
{
    TestJsonReaderFailure<uint64_t>(INT_VALUE_STRING);
    TestJsonReaderSuccess<uint64_t>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonReaderFailure<uint64_t>(INT64_VALUE_STRING);
    TestJsonReaderSuccess<uint64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
    TestJsonReaderFailure<uint64_t>(DOUBLE_VALUE_STRING);
    TestJsonReaderFailure<uint64_t>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsDoubleType_GetExpectedResult)
{
    TestJsonReaderSuccess<double>(INT_VALUE, INT_VALUE_STRING);
    TestJsonReaderSuccess<double>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonReaderSuccess<double>(INT64_VALUE, INT64_VALUE_STRING);
    TestJsonReaderSuccess<double>(UINT64_VALUE, UINT64_VALUE_STRING);
    TestJsonReaderSuccess<double>(DOUBLE_VALUE, DOUBLE_VALUE_STRING);
    TestJsonReaderFailure<double>(STRING_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsObjectType_GetExpectedResult)
{
    const OBJECT_TYPE OBJECT_VALUE{"s"};
    TestJsonReaderSuccess<OBJECT_TYPE>(OBJECT_VALUE, OBJECT_VALUE_JSON);
    TestJsonReaderFailure<OBJECT_TYPE>(STRING_VALUE_JSON);
    TestJsonReaderFailure<OBJECT_TYPE>(ARRAY_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsArrayType_GetExpectedResult)
{
    TestJsonReaderFailure<ARRAY_TYPE>(OBJECT_VALUE_JSON);
    TestJsonReaderFailure<ARRAY_TYPE>(STRING_VALUE_JSON);
    const ARRAY_TYPE ARRAY_VALUE{"a", "b", "c"};
    TestJsonReaderSuccess<ARRAY_TYPE>(ARRAY_VALUE, ARRAY_VALUE_JSON);
}

TEST_F(JsonReaderTest, ReadObject_ReadAsNestedArrayType_GetExpectedResult)
{
    const ARRAY_OF_ARRAY_TYPE ARRAY_OF_ARRAY_VALUE{{"a1", "b1", "c1"}, {"a2", "b2", "c2"}};
    TestJsonReaderSuccess<ARRAY_OF_ARRAY_TYPE>(ARRAY_OF_ARRAY_VALUE, ARRAY_OF_ARRAY_VALUE_JSON);
    const ARRAY_OF_OBJECT_TYPE ARRAY_OF_OBJECT_VALUE{{"s1"}, {"s2"}};
    TestJsonReaderSuccess<ARRAY_OF_OBJECT_TYPE>(ARRAY_OF_OBJECT_VALUE, ARRAY_OF_OBJECT_VALUE_JSON);
}

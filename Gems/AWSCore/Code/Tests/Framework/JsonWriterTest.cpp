/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCoreTestingUtils;

using OBJECT_TYPE = TestObject<AZStd::string>;
using ARRAY_TYPE = AZStd::vector<AZStd::string>;
using JsonWriterTest = UnitTest::ScopedAllocatorSetupFixture;

template<class ValueType>
void TestJsonWriterSuccess(const ValueType& actualValue, const char* valueString)
{
    Aws::StringStream stringStream{};
    AWSCore::JsonOutputStream jsonStream{stringStream};

    TestObject<ValueType> object;
    object.value = actualValue;

    AWSCore::JsonWriter writer{jsonStream};

    bool ok = writer.Write(object);

    Aws::String actualJson = stringStream.str();

    ASSERT_TRUE(ok);

    AZStd::string expectedJson = TestObjectJson(valueString);

    puts(actualJson.c_str());
    puts(expectedJson.c_str());

    ASSERT_EQ(actualJson, expectedJson.c_str());
}

TEST_F(JsonWriterTest, Write_WriteAsDifferentTypes_GetExpectedResult)
{
    TestJsonWriterSuccess<AZStd::string>(STRING_VALUE, STRING_VALUE_JSON);
    TestJsonWriterSuccess<bool>(BOOL_VALUE, BOOL_VALUE_STRING);
    TestJsonWriterSuccess<int>(INT_VALUE, INT_VALUE_STRING);
    TestJsonWriterSuccess<unsigned>(UINT_VALUE, UINT_VALUE_STRING);
    TestJsonWriterSuccess<int64_t>(INT64_VALUE, INT64_VALUE_STRING);
    TestJsonWriterSuccess<uint64_t>(UINT64_VALUE, UINT64_VALUE_STRING);
    const OBJECT_TYPE OBJECT_VALUE{"s"};
    TestJsonWriterSuccess<OBJECT_TYPE>(OBJECT_VALUE, OBJECT_VALUE_JSON);
    const ARRAY_TYPE ARRAY_VALUE{"a", "b", "c"};
    TestJsonWriterSuccess<ARRAY_TYPE>(ARRAY_VALUE, ARRAY_VALUE_JSON);
}

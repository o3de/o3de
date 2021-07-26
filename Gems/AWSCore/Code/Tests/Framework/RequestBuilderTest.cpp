/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Framework/RequestBuilder.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;
using namespace AWSCoreTestingUtils;

using RequestBuilderTest = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(RequestBuilderTest, WriteJsonBodyParameter_UseTestJsonBody_GetExpectedValue)
{
    TestObject<AZStd::string> object;
    object.value = STRING_VALUE;

    RequestBuilder testRequestBuilder;
    testRequestBuilder.WriteJsonBodyParameter(object);

    auto stream = testRequestBuilder.GetBodyContent();
    ASSERT_EQ(stream->str(), TestObjectJson(STRING_VALUE_JSON).c_str());
}

TEST_F(RequestBuilderTest, SetHttpMethod_UsePatchMethod_GetExpectedValue)
{
    RequestBuilder testRequestBuilder;
    testRequestBuilder.SetHttpMethod(Aws::Http::HttpMethod::HTTP_PATCH);
    ASSERT_EQ(testRequestBuilder.GetHttpMethod(), Aws::Http::HttpMethod::HTTP_PATCH);
}

TEST_F(RequestBuilderTest, SetErrorMessage_UseTestMessage_GetExpectedValue)
{
    RequestBuilder testRequestBuilder;
    testRequestBuilder.SetErrorMessage("test");
    ASSERT_EQ(testRequestBuilder.GetErrorMessage(), "test");
}

template<class ValueType>
void TestRequestBuilderSetPathParameter(const ValueType& expectedValue, const char* valueString)
{
    RequestBuilder testRequestBuilder;
    testRequestBuilder.SetRequestUrl("http://test/{param}/test");
    testRequestBuilder.SetPathParameter("{param}", expectedValue);
    ASSERT_EQ(testRequestBuilder.GetRequestUrl(), AZStd::string::format("http://test/%s/test", valueString).c_str());
}

TEST_F(RequestBuilderTest, SetPathParameter_HaveDifferentTypeParameters_GetExpectedUrl)
{
    TestRequestBuilderSetPathParameter(STRING_VALUE, STRING_VALUE_STRING);
    TestRequestBuilderSetPathParameter(BOOL_VALUE, BOOL_VALUE_STRING);
    TestRequestBuilderSetPathParameter(CHARPTR_VALUE, CHARPTR_VALUE_STRING);
    TestRequestBuilderSetPathParameter(INT_VALUE, INT_VALUE_STRING);
    TestRequestBuilderSetPathParameter(UINT_VALUE, UINT_VALUE_STRING);
    TestRequestBuilderSetPathParameter(INT64_VALUE, INT64_VALUE_STRING);
    TestRequestBuilderSetPathParameter(UINT64_VALUE, UINT64_VALUE_STRING);
    TestRequestBuilderSetPathParameter(UNESCAPED, ESCAPED);
}

template<class ValueType>
void TestRequestBuilderAddQueryParameter(const ValueType& expectedValue, const char* valueString)
{
    RequestBuilder testRequestBuilder;
    testRequestBuilder.SetRequestUrl("http://test");
    testRequestBuilder.AddQueryParameter("param", expectedValue);
    ASSERT_EQ(testRequestBuilder.GetRequestUrl(), AZStd::string::format("http://test?param=%s", valueString).c_str());
}

TEST_F(RequestBuilderTest, AddQueryParameter_HaveDifferentTypeParameters_GetExpectedUrl)
{
    TestRequestBuilderAddQueryParameter(STRING_VALUE, STRING_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(BOOL_VALUE, BOOL_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(CHARPTR_VALUE, CHARPTR_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(INT_VALUE, INT_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(UINT_VALUE, UINT_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(INT64_VALUE, INT64_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(UINT64_VALUE, UINT64_VALUE_STRING);
    TestRequestBuilderAddQueryParameter(UNESCAPED, ESCAPED);
}

TEST_F(RequestBuilderTest, AddQueryParameter_HaveMultipeQueryParameters_GetExpectedUrl)
{
    RequestBuilder testRequestBuilder;
    Aws::String expectedUrl = testRequestBuilder.GetRequestUrl();
    expectedUrl.append("?p1=s1&p2=s2");

    testRequestBuilder.AddQueryParameter("p1", "s1");
    testRequestBuilder.AddQueryParameter("p2", "s2");

    ASSERT_EQ(testRequestBuilder.GetRequestUrl(), expectedUrl);
}

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

#include <AzTest/AzTest.h>

#include <AWSCoreBus.h>
#include <Framework/ServiceRequestJob.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

using ServiceRequestJobTest = UnitTest::ScopedAllocatorSetupFixture;

#define TEST_SERVICE_REQUEST(SERVICE_NAME, METHOD, PATH) \
    static const char* Path() { return PATH; } \
    static HttpMethod Method() { return METHOD; }

TEST_F(ServiceRequestJobTest, ServiceRequest_DefineFeatureGemTestServiceRequest_GetExpectedFieldValue)
{
    class MyTestServiceRequest
        : public AWSCore::ServiceRequest
    {
    public:
        TEST_SERVICE_REQUEST(MyTestService, HttpMethod::HTTP_POST, "/test1");
    };

    MyTestServiceRequest testRequest;
    EXPECT_TRUE(testRequest.Method() == ServiceRequest::HttpMethod::HTTP_POST);
    EXPECT_TRUE(AZStd::string(testRequest.Path()) == "/test1");
    EXPECT_TRUE(testRequest.UseAWSCredentials());
}

TEST_F(ServiceRequestJobTest, ServiceRequest_DefineCustomTestServiceRequest_GetExpectedFieldValue)
{
    class MyTestServiceRequest
        : public AWSCore::ServiceRequest
    {
    public:
        TEST_SERVICE_REQUEST(MyTestService, HttpMethod::HTTP_PUT, "/test2");
    };

    MyTestServiceRequest testRequest;
    EXPECT_TRUE(testRequest.Method() == ServiceRequest::HttpMethod::HTTP_PUT);
    EXPECT_TRUE(AZStd::string(testRequest.Path()) == "/test2");
    EXPECT_TRUE(testRequest.UseAWSCredentials());
}

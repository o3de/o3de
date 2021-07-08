/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Framework/HttpRequestJob.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class HttpRequestJobTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
    void SetUp() override
    {
        HttpRequestJob::StaticInit();
    }

    void TearDown() override
    {
        HttpRequestJob::StaticShutdown();
    }
};

TEST_F(HttpRequestJobTest, StringToHttpMethod_HttpMethodToString)
{
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_GET, *HttpRequestJob::StringToHttpMethod("GET"));
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_POST, *HttpRequestJob::StringToHttpMethod("POST"));
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_DELETE, *HttpRequestJob::StringToHttpMethod("DELETE"));
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_PUT, *HttpRequestJob::StringToHttpMethod("PUT"));
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_HEAD, *HttpRequestJob::StringToHttpMethod("HEAD"));
    EXPECT_EQ(HttpRequestJob::HttpMethod::HTTP_PATCH, *HttpRequestJob::StringToHttpMethod("PATCH"));
    EXPECT_FALSE(HttpRequestJob::StringToHttpMethod("Foo"));

    EXPECT_TRUE(strcmp("GET", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_GET)) == 0);
    EXPECT_TRUE(strcmp("POST", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_POST)) == 0);
    EXPECT_TRUE(strcmp("DELETE", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_DELETE)) == 0);
    EXPECT_TRUE(strcmp("PUT", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_PUT)) == 0);
    EXPECT_TRUE(strcmp("HEAD", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_HEAD)) == 0);
    EXPECT_TRUE(strcmp("PATCH", HttpRequestJob::HttpMethodToString(Aws::Http::HttpMethod::HTTP_PATCH)) == 0);
}

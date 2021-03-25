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

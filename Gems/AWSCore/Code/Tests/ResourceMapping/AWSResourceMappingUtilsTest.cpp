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

#include <ResourceMapping/AWSResourceMappingUtils.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

static constexpr const char TEST_VALID_RESTAPI_ID[] = "1234567890";
static constexpr const char TEST_VALID_RESTAPI_REGION[] = "us-west-2";
static constexpr const char TEST_VALID_RESTAPI_CHINA_REGION[] = "cn-north-1";
static constexpr const char TEST_VALID_RESTAPI_STAGE[] = "prod";

using AWSResourceMappingUtilsTest = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(AWSResourceMappingUtilsTest, FormatRESTApiUrl_PassingValidArguments_ReturnExpectedResult)
{
    AZStd::string expectedUrl = AZStd::string::format("https://%s.execute-api.%s.amazonaws.com/%s",
        TEST_VALID_RESTAPI_ID, TEST_VALID_RESTAPI_REGION, TEST_VALID_RESTAPI_STAGE);
    auto actualUrl =
        AWSResourceMappingUtils::FormatRESTApiUrl(TEST_VALID_RESTAPI_ID, TEST_VALID_RESTAPI_REGION, TEST_VALID_RESTAPI_STAGE);

    EXPECT_TRUE(actualUrl == expectedUrl);
}

TEST_F(AWSResourceMappingUtilsTest, FormatRESTApiUrl_PassingValidChinaRegion_ReturnExpectedResult)
{
    AZStd::string expectedUrl = AZStd::string::format("https://%s.execute-api.%s.amazonaws.com.cn/%s",
        TEST_VALID_RESTAPI_ID, TEST_VALID_RESTAPI_CHINA_REGION, TEST_VALID_RESTAPI_STAGE);
    auto actualUrl =
        AWSResourceMappingUtils::FormatRESTApiUrl(TEST_VALID_RESTAPI_ID, TEST_VALID_RESTAPI_CHINA_REGION, TEST_VALID_RESTAPI_STAGE);

    EXPECT_TRUE(actualUrl == expectedUrl);
}

TEST_F(AWSResourceMappingUtilsTest, FormatRESTApiUrl_PassingInvalidRESTApiId_ReturnEmptyResult)
{
    auto actualUrl = AWSResourceMappingUtils::FormatRESTApiUrl("", TEST_VALID_RESTAPI_REGION, TEST_VALID_RESTAPI_STAGE);

    EXPECT_TRUE(actualUrl == "");
}

TEST_F(AWSResourceMappingUtilsTest, FormatRESTApiUrl_PassingInvalidRESTApiRegion_ReturnEmptyResult)
{
    auto actualUrl = AWSResourceMappingUtils::FormatRESTApiUrl(TEST_VALID_RESTAPI_ID, "", TEST_VALID_RESTAPI_STAGE);

    EXPECT_TRUE(actualUrl == "");
}

TEST_F(AWSResourceMappingUtilsTest, FormatRESTApiUrl_PassingInvalidRESTApiStage_ReturnEmptyResult)
{
    auto actualUrl = AWSResourceMappingUtils::FormatRESTApiUrl(TEST_VALID_RESTAPI_ID, TEST_VALID_RESTAPI_REGION, "");

    EXPECT_TRUE(actualUrl == "");
}

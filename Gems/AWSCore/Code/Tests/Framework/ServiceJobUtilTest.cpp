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

#include <Framework/ServiceJobUtil.h>
#include <TestFramework/AWSCoreFixture.h>

using ServiceJobUtilTest = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(ServiceJobUtilTest, DetermineRegionFromRequestUrl_DefaultUrlFormat_Success)
{
    Aws::String defaultUrl = "https://rest-api-id.execute-api.region1.amazonaws.com/stage/path";
    Aws::String region = AWSCore::DetermineRegionFromServiceUrl(defaultUrl);
    EXPECT_EQ(region, Aws::String("region1"));
}

TEST_F(ServiceJobUtilTest, DetermineRegionFromRequestUrl_CustomUrlFormat_Success)
{
    Aws::String alternativeUrl = "https://custom_domain_name/region2.stage.rest-api-id/path";
    Aws::String region = AWSCore::DetermineRegionFromServiceUrl(alternativeUrl);
    EXPECT_EQ(region, Aws::String("region2"));
}

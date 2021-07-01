/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

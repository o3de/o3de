/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Framework/Util.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCoreTestingUtils;

using FrameworkUtilTest = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(FrameworkUtilTest, ToAwsString_UseAzString_GetExpectedAwsString)
{
    const char* EXPECTED_STRING = "122334556454ABfdhfdjfdf";
    AZStd::string testString(EXPECTED_STRING);

    const Aws::String converted = AWSCore::Util::ToAwsString(testString);

    EXPECT_TRUE(strcmp(testString.c_str(), converted.c_str()) == 0);
}

TEST_F(FrameworkUtilTest, toAzString_UseAwsString_GetExpectedAzString)
{
    const char* EXPECTED_STRING = "122334556454ABfdhfdjfdf";
    Aws::String testString(EXPECTED_STRING);

    const AZStd::string converted = AWSCore::Util::ToAZString(testString);

    EXPECT_TRUE(strcmp(testString.c_str(), converted.c_str()) == 0);
}

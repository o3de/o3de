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

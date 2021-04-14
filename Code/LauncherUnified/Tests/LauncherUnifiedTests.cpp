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

#include "../Launcher.h"

class UnifiedLauncherTestFixture
    : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};



TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoAddArgument_NoCommandLineFunctions_Success)
{
    O3DELauncher::PlatformMainInfo test;

    EXPECT_STREQ(test.m_commandLine, "");
    EXPECT_EQ(test.m_argC, 0);
}

TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoAddArgument_ValidParams_Success)
{
    O3DELauncher::PlatformMainInfo test;

    const char* testArguments[] = { "-arg", "value1", "-arg2", "value2", "-argspace", "value one"};
    for (const char* testArgument : testArguments)
    {
        test.AddArgument(testArgument);
    }

    EXPECT_STREQ(test.m_commandLine, "-arg value1 -arg2 value2 -argspace \"value one\"");
    EXPECT_EQ(test.m_argC, 6);
    EXPECT_STREQ(test.m_argV[0], "-arg");
    EXPECT_STREQ(test.m_argV[1], "value1");
    EXPECT_STREQ(test.m_argV[2], "-arg2");
    EXPECT_STREQ(test.m_argV[3], "value2");
    EXPECT_STREQ(test.m_argV[4], "-argspace");
    EXPECT_STREQ(test.m_argV[5], "value one");
}


TEST_F(UnifiedLauncherTestFixture, PlatformMainInfoCopyCommandLineArgCArgV_ValidParams_Success)
{
    O3DELauncher::PlatformMainInfo test;

    const char* constTestArguments[] = { "-arg", "value1", "-arg2", "value2", "-argspace", "value one" };
    char** testArguments = const_cast<char**>(constTestArguments);
    int testArgumentCount = AZ_ARRAY_SIZE(constTestArguments);

    test.CopyCommandLine(testArgumentCount,testArguments);

    EXPECT_STREQ(test.m_commandLine, "-arg value1 -arg2 value2 -argspace \"value one\"");
    EXPECT_EQ(test.m_argC, 6);
    EXPECT_STREQ(test.m_argV[0], "-arg");
    EXPECT_STREQ(test.m_argV[1], "value1");
    EXPECT_STREQ(test.m_argV[2], "-arg2");
    EXPECT_STREQ(test.m_argV[3], "value2");
    EXPECT_STREQ(test.m_argV[4], "-argspace");
    EXPECT_STREQ(test.m_argV[5], "value one");
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

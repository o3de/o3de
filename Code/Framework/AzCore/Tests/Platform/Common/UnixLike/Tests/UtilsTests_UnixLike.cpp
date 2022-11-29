/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTest
{
    class UtilsUnixLikeTestFixture
        : public LeakDetectionFixture
    {
    };

    TEST_F(UtilsUnixLikeTestFixture, ConvertToAbsolutePath_OnRelativePath_Succeeds)
    {
        AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath("./");
        EXPECT_TRUE(absolutePath);
    }

    TEST_F(UtilsUnixLikeTestFixture, ConvertToAbsolutePath_OnAbsolutePath_Succeeds)
    {
        char executableDirectory[AZ::IO::MaxPathLength];
        AZ::Utils::ExecutablePathResult result = AZ::Utils::GetExecutableDirectory(executableDirectory, AZ_ARRAY_SIZE(executableDirectory));
        EXPECT_EQ(AZ::Utils::ExecutablePathResult::Success, result);
        AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath(executableDirectory);
        ASSERT_TRUE(absolutePath);
        EXPECT_STRCASEEQ(executableDirectory, absolutePath->c_str());
    }

    TEST_F(UtilsUnixLikeTestFixture, ConvertToAbsolutePath_OnNonExistentPath_Succeeds)
    {
        AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath(
            "_PathWhichShouldNotExistButIsOkIfItDoesExist");
        ASSERT_TRUE(absolutePath);
    }
}

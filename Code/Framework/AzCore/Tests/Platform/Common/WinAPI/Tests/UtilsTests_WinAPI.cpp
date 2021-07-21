/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTest
{
    class UtilsTestFixture
        : public ScopedAllocatorSetupFixture
    {
    };

    TEST_F(UtilsTestFixture, GetExecutablePath_ReturnsValidPath)
    {
        char executablePath[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
        ASSERT_EQ(AZ::Utils::ExecutablePathResult::Success, result.m_pathStored);
        EXPECT_TRUE(result.m_pathIncludesFilename);
        EXPECT_GT(strlen(executablePath), 0);
    }

    TEST_F(UtilsTestFixture, GetExecutablePath_ReturnsBufferSizeTooLarge)
    {
        char executablePath[1];
        AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, 1);
        EXPECT_EQ(AZ::Utils::ExecutablePathResult::BufferSizeNotLargeEnough, result.m_pathStored);
    }

    TEST_F(UtilsTestFixture, GetExecutableDirectory_ReturnsValidDirectory)
    {
        char executablePath[AZ_MAX_PATH_LEN];
        char executableDirectory[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePathReturnType executablePathResult = AZ::Utils::GetExecutablePath(executablePath, AZ_ARRAY_SIZE(executablePath));
        AZ::Utils::ExecutablePathResult executableDirectoryResult = AZ::Utils::GetExecutableDirectory(executableDirectory, AZ_ARRAY_SIZE(executableDirectory));
        ASSERT_EQ(AZ::Utils::ExecutablePathResult::Success, executableDirectoryResult);

        EXPECT_TRUE(executablePathResult.m_pathIncludesFilename);
        EXPECT_GT(strlen(executablePath), 0);
        EXPECT_TRUE(AZStd::string_view(executablePath).starts_with(executableDirectory));
        EXPECT_LT(strlen(executableDirectory), strlen(executablePath));
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnInvalidPath_Fails)
    {
        AZ::IO::FixedMaxPathString invalidPath{ "Z:\\" };
        invalidPath.append(invalidPath.max_size() - invalidPath.size(), '@');
        EXPECT_FALSE(AZ::Utils::ConvertToAbsolutePath(invalidPath));
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnRelativePath_Succeeds)
    {
        AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath("./");
        EXPECT_TRUE(absolutePath);
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnAbsolutePath_Succeeds)
    {
        char executableDirectory[AZ::IO::MaxPathLength];
        AZ::Utils::ExecutablePathResult result = AZ::Utils::GetExecutableDirectory(executableDirectory, AZ_ARRAY_SIZE(executableDirectory));
        EXPECT_EQ(AZ::Utils::ExecutablePathResult::Success, result);
        AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath(executableDirectory);
        ASSERT_TRUE(absolutePath);
        EXPECT_STRCASEEQ(executableDirectory, absolutePath->c_str());
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        : public LeakDetectionFixture
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

    TEST_F(UtilsTestFixture, GetEnv_CanReadEnvironment_Succeeds)
    {
        // First set an env value using SetEnv
        constexpr const char* envKey = "O3DE_TEST_KEY";
        AZStd::fixed_string<1024> expectedValue(512, 'a');
        EXPECT_TRUE(AZ::Utils::SetEnv(envKey, expectedValue.c_str(), true));

        AZStd::fixed_string<1024> testValue;
        auto StoreEnvironmentValue = [](char* buffer, size_t size) -> size_t
        {
            auto getEnvOutcome = AZ::Utils::GetEnv(AZStd::span(buffer, size), envKey);
            return getEnvOutcome ? getEnvOutcome.GetValue().size() : 0;
        };
        // Resize and overwrite will update the fixed_string to point at the environment variable value
        testValue.resize_and_overwrite(testValue.capacity(), StoreEnvironmentValue);
        EXPECT_EQ(expectedValue, testValue);

        // Make sure to unset the key at the end of the test
        EXPECT_TRUE(AZ::Utils::UnsetEnv(envKey));
    }

    TEST_F(UtilsTestFixture, GetEnv_ReadsUnsetEnvKey_Fails)
    {
        constexpr const char* envKey = "_~~~~O3DE_TEST_KEY_FOO$#%";
        char testValue[1024];
        auto getEnvOutcome = AZ::Utils::GetEnv(AZStd::span(testValue), envKey);
        EXPECT_FALSE(getEnvOutcome);
        EXPECT_EQ(AZ::Utils::GetEnvErrorCode::EnvNotSet, getEnvOutcome.GetError().m_errorCode);
    }

    TEST_F(UtilsTestFixture, GetEnv_WithSpanSizeThatLessThanEnvValueSize_Fails)
    {
        constexpr const char* envKey = "O3DE_TEST_KEY";
        AZStd::fixed_string<1024> expectedValue(512, 'a');
        EXPECT_TRUE(AZ::Utils::SetEnv(envKey, expectedValue.c_str(), true));

        // Initialize get env outcome to failure
        AZ::Utils::GetEnvOutcome getEnvOutcome{ AZStd::unexpect };
        AZStd::fixed_string<1024> testValue;
        auto StoreEnvironmentValue = [&getEnvOutcome](char* buffer, size_t size) -> size_t
        {
            // Negate the GetEnv call and store it in the captured variable
            // to validate the call has failed
            getEnvOutcome = AZ::Utils::GetEnv(AZStd::span(buffer, size), envKey);
            return getEnvOutcome ? getEnvOutcome.GetValue().size() : 0;
        };
        // The testValue string is explicitly being resized to be less than the environment variable value size
        testValue.resize_and_overwrite(expectedValue.size() / 2, StoreEnvironmentValue);

        EXPECT_FALSE(getEnvOutcome);
        EXPECT_EQ(AZ::Utils::GetEnvErrorCode::BufferTooSmall, getEnvOutcome.GetError().m_errorCode);
        EXPECT_EQ(512, getEnvOutcome.GetError().m_requiredSize);
        EXPECT_NE(expectedValue, testValue);

        // Now try to retrieve the environment variable value again this time with a buffer
        // that can fit all the characters
        testValue.resize_and_overwrite(getEnvOutcome.GetError().m_requiredSize, StoreEnvironmentValue);
        EXPECT_TRUE(getEnvOutcome);
        EXPECT_EQ(expectedValue, testValue);

        // Make sure to unset the key at the end of the test
        EXPECT_TRUE(AZ::Utils::UnsetEnv(envKey));
    }

    TEST_F(UtilsTestFixture, IsEnvSet_WithExistingKey_Succeeds)
    {
        constexpr const char* envKey = "O3DE_TEST_KEY";
        AZStd::fixed_string<1024> expectedValue(512, 'a');
        EXPECT_TRUE(AZ::Utils::SetEnv(envKey, expectedValue.c_str(), true));
        EXPECT_TRUE(AZ::Utils::IsEnvSet(envKey));

        // Make sure to unset the key at the end of the test
        EXPECT_TRUE(AZ::Utils::UnsetEnv(envKey));
    }

    TEST_F(UtilsTestFixture, IsEnvSet_WithNonExistentKey_Fails)
    {
        constexpr const char* envKey = "_~~~~O3DE_TEST_KEY_FOO$#%";
        EXPECT_FALSE(AZ::Utils::IsEnvSet(envKey));
    }
}

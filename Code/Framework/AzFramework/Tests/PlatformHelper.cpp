/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Platform/PlatformDefaults.h>

class PlatformHelperTest
    : public UnitTest::LeakDetectionFixture
{
public:

};

TEST_F(PlatformHelperTest, SinglePlatformFlags_PlatformId_Valid)
{
    AzFramework::PlatformFlags platform = AzFramework::PlatformFlags::Platform_PC;
    auto platforms = AzFramework::PlatformHelper::GetPlatforms(platform);
    EXPECT_EQ(platforms.size(), 1);
    EXPECT_EQ(platforms[0], "pc");
}

TEST_F(PlatformHelperTest, MultiplePlatformFlags_PlatformId_Valid)
{
    AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID;
    auto platforms = AzFramework::PlatformHelper::GetPlatforms(platformFlags);
    EXPECT_EQ(platforms.size(), 2);
    EXPECT_EQ(platforms[0], "pc");
    EXPECT_EQ(platforms[1], "android");
}

TEST_F(PlatformHelperTest, SpecialAllFlag_PlatformId_Valid)
{
    AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_ALL;
    auto platforms = AzFramework::PlatformHelper::GetPlatformsInterpreted(platformFlags);
    EXPECT_EQ(platforms.size(), AzFramework::NumPlatforms);
    EXPECT_THAT(platforms, testing::UnorderedElementsAre("pc", "linux", "android", "ios", "mac", "provo", "salem", "jasper", "server"));
}

TEST_F(PlatformHelperTest, SpecialAllClientFlag_PlatformId_Valid)
{
    AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_ALL_CLIENT;
    auto platforms = AzFramework::PlatformHelper::GetPlatformsInterpreted(platformFlags);
    EXPECT_EQ(platforms.size(), AzFramework::NumClientPlatforms);
    EXPECT_THAT(platforms, testing::UnorderedElementsAre("pc", "linux", "android", "ios", "mac", "provo", "salem", "jasper"));
}

TEST_F(PlatformHelperTest, InvalidPlatformFlags_PlatformId_Empty)
{
    AZ::u32 platformFlags = 1 << 20; // Currently we do not have this bit set indicating a valid platform.
    auto platforms = AzFramework::PlatformHelper::GetPlatforms(static_cast<AzFramework::PlatformFlags>(platformFlags));
    EXPECT_EQ(platforms.size(), 0);
}

TEST_F(PlatformHelperTest, GetPlatformName_Valid_OK)
{
    AZStd::string platformName = AzFramework::PlatformHelper::GetPlatformName(AzFramework::PlatformId::PC);
    EXPECT_EQ(platformName, AzFramework::PlatformPC);
}

TEST_F(PlatformHelperTest, GetPlatformName_Invalid_OK)
{
    AZStd::string platformName = AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(-1));
    EXPECT_TRUE(platformName.compare("invalid") == 0);
}

TEST_F(PlatformHelperTest, GetPlatformIndexByName_Valid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName(AzFramework::PlatformPC);
    EXPECT_EQ(platformIndex, AzFramework::PlatformId::PC);
}

TEST_F(PlatformHelperTest, GetPlatformIndexByName_Invalid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName("dummy");
    EXPECT_EQ(platformIndex, AzFramework::PlatformId::Invalid);
}

TEST_F(PlatformHelperTest, GetServerPlatformIndexByName_Valid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName(AzFramework::PlatformServer);
    EXPECT_EQ(platformIndex, AzFramework::PlatformId::SERVER);
}

TEST_F(PlatformHelperTest, GetPlatformIdByName_Valid_OK)
{
    AzFramework::PlatformId platformId = AzFramework::PlatformHelper::GetPlatformIdFromName(AzFramework::PlatformPC);
    EXPECT_EQ(platformId, AzFramework::PlatformId::PC);
}

TEST_F(PlatformHelperTest, GetPlatformIdName_Invalid_OK)
{
    AzFramework::PlatformId platformId = AzFramework::PlatformHelper::GetPlatformIdFromName("dummy");
    EXPECT_EQ(platformId, AzFramework::PlatformId::Invalid);
}

TEST_F(PlatformHelperTest, AppendPlatformCodeNames_ByValidName_OK)
{
    AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
    AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, AzFramework::PlatformPC);
    EXPECT_THAT(platformCodes, testing::Pointwise(testing::Eq(), {AzFramework::PlatformCodeNameWindows}));
}

TEST_F(PlatformHelperTest, AppendPlatformCodeNames_ByInvalidName_OK)
{
    AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
    AZ_TEST_START_TRACE_SUPPRESSION;
    AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, "dummy");
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    EXPECT_TRUE(platformCodes.empty());
}

TEST_F(PlatformHelperTest, AppendPlatformCodeNames_ByValidId_OK)
{
    AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
    AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, AzFramework::PlatformId::PC);
    EXPECT_THAT(platformCodes, testing::Pointwise(testing::Eq(), {AzFramework::PlatformCodeNameWindows}));
}

TEST_F(PlatformHelperTest, AppendPlatformCodeNames_ByInvalidId_OK)
{
    AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
    AZ_TEST_START_TRACE_SUPPRESSION;
    AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, AzFramework::PlatformId::Invalid);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    EXPECT_TRUE(platformCodes.empty());
}

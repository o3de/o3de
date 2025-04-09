/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetSeedUtil.h>

#include "AssetValidationTestShared.h"
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/Asset/AssetSeedList.h>
#include <AzFramework/Gem/GemInfo.h>

namespace UnitTest
{
    bool AssetValidationTest::CreateDummyFile(const AZ::IO::Path& path, AZStd::string_view contents) const
    {
        AZ::IO::FileIOStream fileStream(path.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeCreatePath);

        return fileStream.Write(contents.size(), contents.data()) != 0;
    }

    TEST_F(AssetValidationTest, DefaultSeedList_ReturnsExpectedSeedLists)
    {
        AZStd::vector<AzFramework::GemInfo> gemInfo;


        const AZ::IO::Path gemSeedList = AZ::IO::Path(m_tempDir.GetDirectory()) / "mockGem" / AzFramework::GemInfo::GetGemAssetFolder()
            / AZ::IO::Path("seedList").ReplaceExtension(AssetValidation::AssetSeed::SeedFileExtension);

        const AZ::IO::Path engineSeedList = AZ::IO::Path(m_tempDir.GetDirectory()) / "Assets/Engine"
            / AZ::IO::Path("SeedAssetList").ReplaceExtension(AssetValidation::AssetSeed::SeedFileExtension);

        const AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
        ASSERT_FALSE(projectName.empty());
        const AZ::IO::Path projectSeedList = AZ::IO::Path(m_tempDir.GetDirectory()) / projectName
            / AZ::IO::Path("SeedAssetList").ReplaceExtension(AssetValidation::AssetSeed::SeedFileExtension);

        ASSERT_TRUE(CreateDummyFile(gemSeedList, "Mock Gem Seed List"));
        ASSERT_TRUE(CreateDummyFile(engineSeedList, "Engine Seed List"));

        ASSERT_TRUE(CreateDummyFile(projectSeedList, "Project Seed List"));

        AzFramework::GemInfo mockGem("MockGem");
        mockGem.m_absoluteSourcePaths.push_back(AZ::IO::Path(m_tempDir.GetDirectory()) / "mockGem");
        gemInfo.push_back(mockGem);

        AZStd::vector<AZ::IO::Path> defaultSeedLists{ AZStd::from_range, AssetValidation::AssetSeed::GetDefaultSeedListFiles(gemInfo, AzFramework::PlatformFlags::Platform_PC) };

        ASSERT_THAT(defaultSeedLists, ::testing::UnorderedElementsAre(gemSeedList, engineSeedList, projectSeedList));
    }

    TEST_F(AssetValidationTest, SeedListDependencyTest_AddAndRemoveList_Success)
    {
        MockValidationComponent testComponent;

        AzFramework::AssetSeedList seedList;
        const char DummyAssetFile[] = "Dummy";
        seedList.push_back({ testComponent.m_assetIds[6], AzFramework::PlatformFlags::Platform_PC, DummyAssetFile });

        testComponent.TestAddSeedsFor(seedList, 2);
        testComponent.SeedMode();

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

        testComponent.TestRemoveSeedsFor(seedList, 0);

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

        testComponent.TestRemoveSeedsFor(seedList, 2);

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
    }


    TEST_F(AssetValidationTest, SeedAssetDependencyTest_AddSingleAsset_DependenciesFound)
    {
        MockValidationComponent testComponent;
        testComponent.AddSeedAssetId(testComponent.m_assetIds[8], 0);
        testComponent.SeedMode();

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath7"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
    }

    TEST_F(AssetValidationTest, SeedListDependencyTest_AddAndRemoveListPaths_Success)
    {
        MockValidationComponent testComponent;

        const char seedListPathName[] = "ValidPath";
        const char invalidSeedListPathName[] = "InvalidPath";

        AzFramework::AssetSeedList seedList;
        const char DummyAssetFile[] = "Dummy";
        seedList.push_back({ testComponent.m_assetIds[6], AzFramework::PlatformFlags::Platform_PC, DummyAssetFile });

        testComponent.m_validSeedList = seedList;
        testComponent.m_validSeedPath = seedListPathName;

        testComponent.TestAddSeedList(invalidSeedListPathName);
        testComponent.SeedMode();

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

        testComponent.TestAddSeedList(seedListPathName);

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

        testComponent.TestRemoveSeedList(invalidSeedListPathName);

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

        testComponent.TestRemoveSeedList(seedListPathName);

        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
        EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);



/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/AssetDebugInfo.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Tests/AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzTest/Utils.h>
#include <Utils/Utils.h>

namespace // anonymous
{
    static const int s_totalAssets = 12;
    static const int s_totalTestPlatforms = 2;

    AZ::Data::AssetId assets[s_totalAssets];
    const char TestSliceAssetPath[] = "test.slice";
    const char SecondTestSliceAssetPath[] = "secondTest.slice";
    const char TestDynamicSliceAssetPath[] = "test.dynamicslice";

    bool Search(const AzToolsFramework::AssetFileInfoList& assetList, const AZ::Data::AssetId& assetId)
    {
        return AZStd::find_if(assetList.m_fileInfoList.begin(), assetList.m_fileInfoList.end(),
            [&](AzToolsFramework::AssetFileInfo fileInfo)
            {
                return fileInfo.m_assetId == assetId;
            });
    }
}


namespace UnitTest
{
    class AssetSeedManagerTest
        : public AllocatorsFixture
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        void SetUp() override
        {
            using namespace AZ::Data;
            constexpr size_t MaxCommandArgsCount = 128;
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            using ArgumentContainer = AZStd::fixed_vector<char*, MaxCommandArgsCount>;
            // The first command line argument is assumed to be the executable name so add a blank entry for it
            ArgumentContainer argContainer{ {} };

            // Append Command Line override for the Project Cache Path
            AZ::IO::Path cacheProjectRootFolder{ m_tempDir.GetDirectory() };
            auto projectCachePathOverride = FixedValueString::format(R"(--project-cache-path="%s")", cacheProjectRootFolder.c_str());
            auto projectPathOverride = FixedValueString{ R"(--project-path=AutomatedTesting)" };
            argContainer.push_back(projectCachePathOverride.data());
            argContainer.push_back(projectPathOverride.data());
            m_application = new ToolsTestApplication("AssetSeedManagerTest", aznumeric_caster(argContainer.size()), argContainer.data());
            m_assetSeedManager = new AzToolsFramework::AssetSeedManager();
            m_assetRegistry = new AzFramework::AssetRegistry();

            m_application->Start(AzFramework::Application::Descriptor());

            // By default @products@ is setup to include the platform at the end. But this test is going to
            // loop over platforms and it will be included as part of the relative path of the file.
            // So the asset folder for these tests have to point to the cache project root folder, which
            // doesn't include the platform.
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", cacheProjectRootFolder.c_str());

            for (int idx = 0; idx < s_totalAssets; idx++)
            {
                assets[idx] = AssetId(AZ::Uuid::CreateRandom(), 0);
                AZ::Data::AssetInfo info;
                info.m_relativePath = AZStd::string::format("asset%d.txt", idx);
                m_assetsPath[idx] = info.m_relativePath;
                info.m_assetId = assets[idx];
                m_assetRegistry->RegisterAsset(assets[idx], info);
            }

            m_testPlatforms[0] = AzFramework::PlatformId::PC;
            m_testPlatforms[1] = AzFramework::PlatformId::ANDROID_ID;

            int platformCount = 0;
            for(auto thisPlatform : m_testPlatforms)
            {
                AZ::IO::Path assetRoot = AzToolsFramework::PlatformAddressedAssetCatalog::GetAssetRootForPlatform(thisPlatform);

                for (int idx = 0; idx < s_totalAssets; idx++)
                {
                    m_assetsPathFull[platformCount][idx] = (assetRoot / m_assetsPath[idx]).Native();
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    if (m_fileStreams[platformCount][idx].Open(m_assetsPathFull[platformCount][idx].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
                    {
                        AZ::IO::SizeType bytesWritten = m_fileStreams[platformCount][idx].Write(m_assetsPath[idx].size(), m_assetsPath[idx].data());
                        EXPECT_EQ(bytesWritten, m_assetsPath[idx].size());
                        m_fileStreams[platformCount][idx].Close();
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
                    }
                    else
                    {
                        GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to create temporary file ( %s ) in AssetSeedManager unit tests.\n", m_assetsPath[idx].c_str()).c_str());
                    }
                }
                ++platformCount;
            }

            AZ::Uuid sourceUUid = AZ::Uuid::CreateRandom();
            AZ::Data::AssetId testSliceAsset = AZ::Data::AssetId(sourceUUid, 0);
            AZ::Data::AssetId testDynamicSliceAsset = AZ::Data::AssetId(sourceUUid, 1);

            AZ::Data::AssetId secondTestSliceAsset = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);

            AZ::Data::AssetInfo dynamicSliceAssetInfo;
            dynamicSliceAssetInfo.m_relativePath = TestDynamicSliceAssetPath;
            dynamicSliceAssetInfo.m_assetId = testDynamicSliceAsset;
            m_testDynamicSliceAssetId = testDynamicSliceAsset;
            m_assetRegistry->RegisterAsset(testDynamicSliceAsset, dynamicSliceAssetInfo);

            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ::IO::FileIOStream dynamicSliceFileIOStream(TestDynamicSliceAssetPath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder

            AZ::Data::AssetInfo sliceAssetInfo;
            sliceAssetInfo.m_relativePath = TestSliceAssetPath;
            sliceAssetInfo.m_assetId = testSliceAsset;
            m_assetRegistry->RegisterAsset(testSliceAsset, sliceAssetInfo);

            AZ::Data::AssetInfo secondSliceAssetInfo;
            secondSliceAssetInfo.m_relativePath = SecondTestSliceAssetPath;
            secondSliceAssetInfo.m_assetId = secondTestSliceAsset;
            m_assetRegistry->RegisterAsset(secondTestSliceAsset, secondSliceAssetInfo);

            AZ_TEST_START_TRACE_SUPPRESSION;
            AZ::IO::FileIOStream sliceFileIOStream(TestSliceAssetPath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder

            // asset0 -> asset1 -> asset2 -> asset4
            //                 --> asset3
            m_assetRegistry->RegisterAssetDependency(assets[0], AZ::Data::ProductDependency(assets[1], 0));
            m_assetRegistry->RegisterAssetDependency(assets[1], AZ::Data::ProductDependency(assets[2], 0));
            m_assetRegistry->RegisterAssetDependency(assets[1], AZ::Data::ProductDependency(assets[3], 0));
            m_assetRegistry->RegisterAssetDependency(assets[2], AZ::Data::ProductDependency(assets[4], 0));

            // asset5 -> asset6 -> asset7
            m_assetRegistry->RegisterAssetDependency(assets[5], AZ::Data::ProductDependency(assets[6], 0));
            m_assetRegistry->RegisterAssetDependency(assets[6], AZ::Data::ProductDependency(assets[7], 0));

            // asset8 -> asset6
            m_assetRegistry->RegisterAssetDependency(assets[8], AZ::Data::ProductDependency(assets[6], 0));

            // asset10 -> asset11
            m_assetRegistry->RegisterAssetDependency(assets[10], AZ::Data::ProductDependency(assets[11], 0));

            // asset11 -> asset10
            m_assetRegistry->RegisterAssetDependency(assets[11], AZ::Data::ProductDependency(assets[10], 0));

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AZ::SerializeContext* context;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            ASSERT_TRUE(context) << "No serialize context.\n";

            AzToolsFramework::AssetSeedManager::Reflect(context);

            // Asset Catalog does not exposes its internal asset registry and the only way to set it is through LoadCatalog API
            // Currently i am serializing the asset registry to disk
            // and invoking the LoadCatalog API to populate the asset catalog created by the azframework app.

            bool useRequestBus = false;
            AzFramework::AssetCatalog assetCatalog(useRequestBus);

            AZStd::string pcCatalogFile = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId::PC);
            AZStd::string androidCatalogFile = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId::ANDROID_ID);

            if (!assetCatalog.SaveCatalog(pcCatalogFile.c_str(), m_assetRegistry))
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to save the asset catalog (PC) file.\n").c_str());
            }

            if (!assetCatalog.SaveCatalog(androidCatalogFile.c_str(), m_assetRegistry))
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to save the asset catalog (ANDROID) file.\n").c_str());
            }

            m_pcCatalog = new AzToolsFramework::PlatformAddressedAssetCatalog(AzFramework::PlatformId::PC);
            m_androidCatalog = new AzToolsFramework::PlatformAddressedAssetCatalog(AzFramework::PlatformId::ANDROID_ID);

            const AZStd::string engroot = AZ::Test::GetEngineRootPath();
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engroot.c_str());
        }

        void TearDown() override
        {
            delete m_assetSeedManager;
            delete m_assetRegistry;
            delete m_pcCatalog;
            delete m_androidCatalog;
            m_application->Stop();
            delete m_application;
        }

        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            auto foundIter = m_assetRegistry->m_assetIdToInfo.find(id);
            if (foundIter != m_assetRegistry->m_assetIdToInfo.end())
            {
                return foundIter->second;
            }

            return AZ::Data::AssetInfo();
        }

        void AssetSeedManager_SaveSeedListFile_FileIsReadOnly()
        {
            AZ::Test::ScopedAutoTempDirectory tempDir;

            constexpr const char fileName[] = "ReadOnlyTestFile.seed";
            AZStd::string filePath = tempDir.Resolve(fileName);

            // Create the file
            EXPECT_TRUE(m_assetSeedManager->Save(filePath));

            // Mark the file Read-Only
            AZ::IO::SystemFile::SetWritable(filePath.c_str(), false);

            // Attempt to save to the same file. Should not be allowed.
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(m_assetSeedManager->Save(filePath));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // One error expected

            // Clean up the test environment
            AZ::IO::SystemFile::SetWritable(filePath.c_str(), true);
            AZ::IO::SystemFile::Delete(filePath.c_str());
        }

        void AssetSeedManager_SaveAssetInfoFile_FileIsReadOnly()
        {
            AZ::Test::ScopedAutoTempDirectory tempDir;

            constexpr const char fileName[] = "ReadOnlyTestFile.assetlist";
            AZStd::string filePath = tempDir.Resolve(fileName);

            // Add a single asset - empty asset list files don't save
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

            // Create the file
            EXPECT_TRUE(m_assetSeedManager->SaveAssetFileInfo(filePath, AzFramework::PlatformFlags::Platform_PC, {}));

            // Mark the file Read-Only
            AZ::IO::SystemFile::SetWritable(filePath.c_str(), false);

            // Attempt to save to the same file. Should not be allowed.
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(m_assetSeedManager->SaveAssetFileInfo(filePath, AzFramework::PlatformFlags::Platform_PC, {}));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // One error expected

            // Clean up the test environment
            AZ::IO::SystemFile::SetWritable(filePath.c_str(), true);
            AZ::IO::SystemFile::Delete(filePath.c_str());
        }

        void ValidateSeedFileExtension_CorrectFileExtension_ExpectSuccess()
        {
            AZStd::string path("some/test/path/file.seed");
            AZ::Outcome<void, AZStd::string> validationOutcome = AzToolsFramework::AssetSeedManager::ValidateSeedFileExtension(path);

            EXPECT_TRUE(validationOutcome.IsSuccess());
        }

        void ValidateSeedFileExtension_IncorrectFileExtension_ExpectFailure()
        {
            AZStd::string path("some/test/path/file.xml");
            AZ::Outcome<void, AZStd::string> validationOutcome = AzToolsFramework::AssetSeedManager::ValidateSeedFileExtension(path);

            EXPECT_FALSE(validationOutcome.IsSuccess());
        }

        void ValidateAssetListFileExtension_CorrectFileExtension_ExpectSuccess()
        {
            AZStd::string path("some/test/path/file.assetlist");
            AZ::Outcome<void, AZStd::string> validationOutcome = AzToolsFramework::AssetFileInfoList::ValidateAssetListFileExtension(path);

            EXPECT_TRUE(validationOutcome.IsSuccess());
        }

        void ValidateAssetListFileExtension_IncorrectFileExtension_ExpectFailure()
        {
            AZStd::string path("some/test/path/file.xml");
            AZ::Outcome<void, AZStd::string> validationOutcome = AzToolsFramework::AssetFileInfoList::ValidateAssetListFileExtension(path);

            EXPECT_FALSE(validationOutcome.IsSuccess());
        }

        void AddPlatformToAllSeeds_SeedsAreValidForPlatform_AllSeedsUpdated()
        {
            // Setup
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[1], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[2], AzFramework::PlatformFlags::Platform_PC);

            // Step we are testing
            m_assetSeedManager->AddPlatformToAllSeeds(AzFramework::PlatformId::ANDROID_ID);

            // Verification
            AzFramework::PlatformFlags expectedPlatformFlags = AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID;
            for (const auto& seedInfo : m_assetSeedManager->GetAssetSeedList())
            {
                EXPECT_EQ(seedInfo.m_platformFlags, expectedPlatformFlags);
            }
        }

        void AddPlatformToAllSeeds_NotAllSeedsAreValidForPlatform_InvalidSeedsNotChanged()
        {
            // Setup
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[1], AzFramework::PlatformFlags::Platform_PC);

            m_androidCatalog->UnregisterAsset(assets[2]);
            m_assetSeedManager->AddSeedAsset(assets[2], AzFramework::PlatformFlags::Platform_PC);

            // Step we are testing
            AZ_TEST_START_TRACE_SUPPRESSION;
            m_assetSeedManager->AddPlatformToAllSeeds(AzFramework::PlatformId::ANDROID_ID);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1); // One error expected

            // Verification
            AzFramework::PlatformFlags expectedPlatformFlags = AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID;
            for (const auto& seedInfo : m_assetSeedManager->GetAssetSeedList())
            {
                if (seedInfo.m_assetId == assets[2])
                {
                    EXPECT_EQ(seedInfo.m_platformFlags, AzFramework::PlatformFlags::Platform_PC);
                }
                else
                {
                    EXPECT_EQ(seedInfo.m_platformFlags, expectedPlatformFlags);
                }
            }
        }

        void RemovePlatformFromAllSeeds_PlatformIsPresentInAllSeeds_PlatformIsRemoved()
        {
            // Setup
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_ANDROID);
            m_assetSeedManager->AddSeedAsset(assets[1], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[1], AzFramework::PlatformFlags::Platform_ANDROID);
            m_assetSeedManager->AddSeedAsset(assets[2], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[2], AzFramework::PlatformFlags::Platform_ANDROID);

            // Step we are testing
            m_assetSeedManager->RemovePlatformFromAllSeeds(AzFramework::PlatformId::ANDROID_ID);

            // Verification
            for (const auto& seedInfo : m_assetSeedManager->GetAssetSeedList())
            {
                EXPECT_EQ(seedInfo.m_platformFlags, AzFramework::PlatformFlags::Platform_PC);
            }
        }

        void RemovePlatformFromAllSeeds_SeedsOnlyHaveOnePlatform_SeedsAreNotChanged()
        {
            // Setup
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[1], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[2], AzFramework::PlatformFlags::Platform_PC);

            // Step we are testing
            m_assetSeedManager->RemovePlatformFromAllSeeds(AzFramework::PlatformId::PC);

            // Verification
            for (const auto& seedInfo : m_assetSeedManager->GetAssetSeedList())
            {
                EXPECT_EQ(seedInfo.m_platformFlags, AzFramework::PlatformFlags::Platform_PC);
            }
        }

        void DependencyValidation_SingleAssetSeed_ListValid()
        {
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

            AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 5);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));

            m_assetSeedManager->RemoveSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC);

            assetList.m_fileInfoList.clear();

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 3);
            EXPECT_TRUE(Search(assetList, assets[5]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));

            m_assetSeedManager->RemoveSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_PC);

            assetList.m_fileInfoList.clear();

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 3);
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));

            assetList.m_fileInfoList.clear();

            m_assetSeedManager->RemoveSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[9], AzFramework::PlatformFlags::Platform_PC);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);
            EXPECT_EQ(assetList.m_fileInfoList.size(), 1);
        }

        void DependencyValidation_MultipleAssetSeed_ListValid()
        {
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC);

            AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[5]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));

            assetList.m_fileInfoList.clear();

            m_assetSeedManager->AddSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_PC);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 9);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[5]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));

            assetList.m_fileInfoList.clear();
            m_assetSeedManager->RemoveSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));
        }

        void DependencyValidation_MultipleAssetSeeds_MultiplePlatformFlags_ListValid()
        {
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID);
            m_assetSeedManager->AddSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID);

            AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[5]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));

            assetList.m_fileInfoList.clear();

            m_assetSeedManager->AddSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 9);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[5]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));

            assetList.m_fileInfoList.clear();
            m_assetSeedManager->RemoveSeedAsset(assets[5], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ANDROID);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));

            // Removing the android flag from the asset should still produce the same result
            m_assetSeedManager->RemoveSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_ANDROID);

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));

            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::ANDROID_ID);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 5);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));

            // Adding the android flag again to the asset
            m_assetSeedManager->AddSeedAsset(assets[8], AzFramework::PlatformFlags::Platform_ANDROID);
            assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::ANDROID_ID);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 8);
            EXPECT_TRUE(Search(assetList, assets[0]));
            EXPECT_TRUE(Search(assetList, assets[1]));
            EXPECT_TRUE(Search(assetList, assets[2]));
            EXPECT_TRUE(Search(assetList, assets[3]));
            EXPECT_TRUE(Search(assetList, assets[4]));
            EXPECT_TRUE(Search(assetList, assets[6]));
            EXPECT_TRUE(Search(assetList, assets[7]));
            EXPECT_TRUE(Search(assetList, assets[8]));
        }

        void DependencyValidation_EmptyAssetSeed_ListValid()
        {
            AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);
            EXPECT_EQ(assetList.m_fileInfoList.size(), 0);
        }

        void DependencyValidation_CyclicAssetSeedDependency_ListValid()
        {

            m_assetSeedManager->AddSeedAsset(assets[10], AzFramework::PlatformFlags::Platform_PC);
            m_assetSeedManager->AddSeedAsset(assets[11], AzFramework::PlatformFlags::Platform_PC);

            AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList.m_fileInfoList.size(), 2);
            EXPECT_TRUE(Search(assetList, assets[10]));
            EXPECT_TRUE(Search(assetList, assets[11]));
        }

        void FileModTimeValidation_SingleAssetSeed_ModTimeChanged()
        {
            int fileIndex = 4;
            m_assetSeedManager->AddSeedAsset(assets[fileIndex], AzFramework::PlatformFlags::Platform_PC);

            AzToolsFramework::AssetFileInfoList assetList1 = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList1.m_fileInfoList.size(), 1);
            EXPECT_TRUE(Search(assetList1, assets[fileIndex]));
            AZ_TEST_START_TRACE_SUPPRESSION;
            if (m_fileStreams[0][fileIndex].Open(m_assetsPathFull[0][fileIndex].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
            {
                AZStd::string fileContent = AZStd::string::format("asset%d.txt", fileIndex);
                AZ::IO::SizeType bytesWritten = m_fileStreams[0][fileIndex].Write(fileContent.size(), fileContent.c_str());
                EXPECT_EQ(bytesWritten, fileContent.size());
                m_fileStreams[0][fileIndex].Close();
                AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
            }

            AzToolsFramework::AssetFileInfoList assetList2 = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList2.m_fileInfoList.size(), 1);
            EXPECT_TRUE(Search(assetList2, assets[fileIndex]));

            EXPECT_EQ(assetList1.m_fileInfoList[0].m_assetId, assetList2.m_fileInfoList[0].m_assetId);
            EXPECT_GE(assetList2.m_fileInfoList[0].m_modificationTime, assetList1.m_fileInfoList[0].m_modificationTime); // file mod time should change

            // file hash should not change
            for (int idx = 0; idx < 5; idx++)
            {
                EXPECT_EQ(assetList1.m_fileInfoList[0].m_hash[idx], assetList2.m_fileInfoList[0].m_hash[idx]);
            }
        }

        void FileHashValidation_SingleAssetSeed_FileHashChanged()
        {
            int fileIndex = 4;
            m_assetSeedManager->AddSeedAsset(assets[fileIndex], AzFramework::PlatformFlags::Platform_PC);

            AzToolsFramework::AssetFileInfoList assetList1 = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList1.m_fileInfoList.size(), 1);
            EXPECT_TRUE(Search(assetList1, assets[fileIndex]));
            AZ_TEST_START_TRACE_SUPPRESSION;
            if (m_fileStreams[0][fileIndex].Open(m_assetsPathFull[0][fileIndex].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
            {
                AZStd::string fileContent = AZStd::string::format("asset%d.txt", fileIndex + 1);// changing file content
                AZ::IO::SizeType bytesWritten = m_fileStreams[0][fileIndex].Write(fileContent.size(), fileContent.c_str());
                EXPECT_EQ(bytesWritten, fileContent.size());
                m_fileStreams[0][fileIndex].Close();
                AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
            }

            AzToolsFramework::AssetFileInfoList assetList2 = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC);

            EXPECT_EQ(assetList2.m_fileInfoList.size(), 1);
            EXPECT_TRUE(Search(assetList2, assets[fileIndex]));

            EXPECT_EQ(assetList1.m_fileInfoList[0].m_assetId, assetList2.m_fileInfoList[0].m_assetId);
            EXPECT_GE(assetList2.m_fileInfoList[0].m_modificationTime, assetList1.m_fileInfoList[0].m_modificationTime);

            bool fileHashDifferent = false;
            // file hash should change since we have modified the file content.
            for (int idx = 0; idx < 5; idx++)
            {
                if (assetList1.m_fileInfoList[0].m_hash[idx] != assetList2.m_fileInfoList[0].m_hash[idx])
                {
                    fileHashDifferent = true;
                    break;
                }
            }

            EXPECT_TRUE(fileHashDifferent);
        }

        void SeedFilePath_UpdatePath_Valid()
        {
            int validFileIndex = 4;
            int invalidFileIndex = 5;
            m_assetSeedManager->AddSeedAsset(assets[validFileIndex], AzFramework::PlatformFlags::Platform_PC, m_assetsPath[invalidFileIndex]);

            const AzFramework::AssetSeedList& oldSeedList = m_assetSeedManager->GetAssetSeedList();

            for (const auto& seedInfo : oldSeedList)
            {
                if (seedInfo.m_assetId == assets[validFileIndex])
                {
                    EXPECT_EQ(seedInfo.m_assetRelativePath, m_assetsPath[invalidFileIndex]);
                }
            }

            m_assetSeedManager->UpdateSeedPath();

            const AzFramework::AssetSeedList& newSeedList = m_assetSeedManager->GetAssetSeedList();

            for (const auto& seedInfo : newSeedList)
            {
                if (seedInfo.m_assetId == assets[validFileIndex])
                {
                    EXPECT_EQ(seedInfo.m_assetRelativePath, m_assetsPath[validFileIndex]);
                }
            }
        }

        void SeedFilePath_RemovePath_Valid()
        {
            m_assetSeedManager->RemoveSeedPath();
            const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

            for (const auto& seedInfo : seedList)
            {
                EXPECT_TRUE(seedInfo.m_assetRelativePath.empty());
            }
        }

        void RemoveSeed_ByAssetId_Valid()
        {
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

            m_assetSeedManager->RemoveSeedAsset(assets[0].ToString<AZStd::string>(), AzFramework::PlatformFlags::Platform_PC);
            const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

            EXPECT_EQ(seedList.size(), 0);
        }

        void RemoveSeed_ByAssetHint_Valid()
        {
            m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

            m_pcCatalog->UnregisterAsset(assets[0]); // Unregister the asset from the asset catalog

            m_assetSeedManager->RemoveSeedAsset(m_assetsPath[0], AzFramework::PlatformFlags::Platform_PC);
            const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();
            EXPECT_EQ(seedList.size(), 0);
        }

        void AddSourceAsset_AddRuntimeAsset_Valid()
        {
            m_assetSeedManager->AddSeedAsset(TestSliceAssetPath, AzFramework::PlatformFlags::Platform_PC);

            const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

            EXPECT_EQ(seedList.size(), 1);
            EXPECT_EQ(seedList[0].m_assetRelativePath, TestDynamicSliceAssetPath);
        }

        void AddSourceAsset_NoRuntimeSlice_Valid()
        {
            m_assetSeedManager->AddSeedAsset(SecondTestSliceAssetPath, AzFramework::PlatformFlags::Platform_PC);

            const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

            EXPECT_EQ(seedList.size(), 0);

        }

        AzToolsFramework::AssetSeedManager* m_assetSeedManager = nullptr;
        AzFramework::AssetRegistry* m_assetRegistry = nullptr;
        ToolsTestApplication* m_application = nullptr;
        AzToolsFramework::PlatformAddressedAssetCatalog* m_pcCatalog = nullptr;
        AzToolsFramework::PlatformAddressedAssetCatalog* m_androidCatalog = nullptr;
        AZ::IO::FileIOStream m_fileStreams[s_totalTestPlatforms][s_totalAssets];
        AzFramework::PlatformId m_testPlatforms[s_totalTestPlatforms];
        AZStd::string m_assetsPath[s_totalAssets];
        AZStd::string m_assetsPathFull[s_totalTestPlatforms][s_totalAssets];
        AZ::Data::AssetId m_testDynamicSliceAssetId;
        UnitTest::ScopedTemporaryDirectory m_tempDir;
    };

    TEST_F(AssetSeedManagerTest, AssetSeedManager_SaveSeedListFile_FileIsReadOnly)
    {
        AssetSeedManager_SaveSeedListFile_FileIsReadOnly();
    }

    TEST_F(AssetSeedManagerTest, AssetSeedManager_SaveAssetInfoFile_FileIsReadOnly)
    {
        AssetSeedManager_SaveAssetInfoFile_FileIsReadOnly();
    }

    TEST_F(AssetSeedManagerTest, ValidateSeedFileExtension_CorrectFileExtension_ExpectSuccess)
    {
        ValidateSeedFileExtension_CorrectFileExtension_ExpectSuccess();
    }

    TEST_F(AssetSeedManagerTest, ValidateSeedFileExtension_IncorrectFileExtension_ExpectFailure)
    {
        ValidateSeedFileExtension_IncorrectFileExtension_ExpectFailure();
    }

    TEST_F(AssetSeedManagerTest, ValidateAssetListFileExtension_CorrectFileExtension_ExpectSuccess)
    {
        ValidateAssetListFileExtension_CorrectFileExtension_ExpectSuccess();
    }

    TEST_F(AssetSeedManagerTest, ValidateAssetListFileExtension_IncorrectFileExtension_ExpectFailure)
    {
        ValidateAssetListFileExtension_IncorrectFileExtension_ExpectFailure();
    }

    TEST_F(AssetSeedManagerTest, AddPlatformToAllSeeds_SeedsAreValidForPlatform_AllSeedsUpdated)
    {
        AddPlatformToAllSeeds_SeedsAreValidForPlatform_AllSeedsUpdated();
    }

    TEST_F(AssetSeedManagerTest, AddPlatformToAllSeeds_NotAllSeedsAreValidForPlatform_InvalidSeedsNotChanged)
    {
        AddPlatformToAllSeeds_NotAllSeedsAreValidForPlatform_InvalidSeedsNotChanged();
    }

    TEST_F(AssetSeedManagerTest, RemovePlatformFromAllSeeds_PlatformIsPresentInAllSeeds_PlatformIsRemoved)
    {
        RemovePlatformFromAllSeeds_PlatformIsPresentInAllSeeds_PlatformIsRemoved();
    }

    TEST_F(AssetSeedManagerTest, RemovePlatformFromAllSeeds_SeedsOnlyHaveOnePlatform_SeedsAreNotChanged)
    {
        RemovePlatformFromAllSeeds_SeedsOnlyHaveOnePlatform_SeedsAreNotChanged();
    }

    TEST_F(AssetSeedManagerTest, DependencyValidation_EmptyAssetSeed_ListValid)
    {
        DependencyValidation_EmptyAssetSeed_ListValid();
    }

    TEST_F(AssetSeedManagerTest, DependencyValidation_SingleAssetSeed_ListValid)
    {
        DependencyValidation_SingleAssetSeed_ListValid();
    }

    TEST_F(AssetSeedManagerTest, DependencyValidation_MultipleAssetSeeds_MultiplePlatformFlags_ListValid)
    {
        DependencyValidation_MultipleAssetSeeds_MultiplePlatformFlags_ListValid();
    }

    TEST_F(AssetSeedManagerTest, DependencyValidation_MultipleAssetSeed_ListValid)
    {
        DependencyValidation_MultipleAssetSeed_ListValid();
    }

    TEST_F(AssetSeedManagerTest, FileModTimeValidation_SingleAssetSeed_ModTimeChanged)
    {
        FileModTimeValidation_SingleAssetSeed_ModTimeChanged();
    }

    TEST_F(AssetSeedManagerTest, FileHashValidation_SingleAssetSeed_FileHashChanged)
    {
        FileHashValidation_SingleAssetSeed_FileHashChanged();
    }

    TEST_F(AssetSeedManagerTest, SeedFilePath_UpdatePath_Valid)
    {
        SeedFilePath_UpdatePath_Valid();
    }

    TEST_F(AssetSeedManagerTest, SeedFilePath_RemovePath_Valid)
    {
        SeedFilePath_RemovePath_Valid();
    }

    TEST_F(AssetSeedManagerTest, RemoveSeed_ByAssetId_Valid)
    {
        RemoveSeed_ByAssetId_Valid();
    }

    TEST_F(AssetSeedManagerTest, RemoveSeed_ByAssetHint_Valid)
    {
        RemoveSeed_ByAssetHint_Valid();
    }

    TEST_F(AssetSeedManagerTest, AddSourceAsset_AddRuntimeAsset_Valid)
    {
        AddSourceAsset_AddRuntimeAsset_Valid();
    }

    TEST_F(AssetSeedManagerTest, AddSourceAsset_NoRuntimeSlice_Valid)
    {
        AddSourceAsset_NoRuntimeSlice_Valid();
    }

    TEST_F(AssetSeedManagerTest, GetDependencyList_ExcludeAsset_IncludesOnlyExpected)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

        AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC, {assets[1]});

        ASSERT_EQ(assetList.m_fileInfoList.size(), 1);
        EXPECT_TRUE(Search(assetList, assets[0]));
    }

    TEST_F(AssetSeedManagerTest, GetDependencyList_ExcludeAssetDebugList_IncludesOnlyExpected)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

        AzToolsFramework::AssetFileDebugInfoList debugList;

        AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC, { assets[1] }, &debugList);

        ASSERT_EQ(assetList.m_fileInfoList.size(), 1);
        EXPECT_TRUE(Search(assetList, assets[0]));
    }

    TEST_F(AssetSeedManagerTest, GetDependencyList_ExcludeSeed_ExcludesEverything)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

        AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC, { assets[0] });

        ASSERT_EQ(assetList.m_fileInfoList.size(), 0);
    }

    TEST_F(AssetSeedManagerTest, GetDependencyList_ExcludeSeedDebugList_ExcludesEverything)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC);

        AzToolsFramework::AssetFileDebugInfoList debugList;

        AzToolsFramework::AssetFileInfoList assetList = m_assetSeedManager->GetDependencyList(AzFramework::PlatformId::PC, { assets[0] }, &debugList);

        ASSERT_EQ(assetList.m_fileInfoList.size(), 0);
    }

    TEST_F(AssetSeedManagerTest, AddSeedAssetForValidPlatforms_AllPlatformsValid_SeedAddedForEveryInputPlatform)
    {
        using namespace AzFramework;
        PlatformFlags validPlatforms = PlatformFlags::Platform_PC | PlatformFlags::Platform_ANDROID;
        AZStd::pair<AZ::Data::AssetId, PlatformFlags> result = m_assetSeedManager->AddSeedAssetForValidPlatforms(TestDynamicSliceAssetPath, validPlatforms);

        // Verify the function outputs
        EXPECT_EQ(m_testDynamicSliceAssetId, result.first);
        EXPECT_EQ(validPlatforms, result.second);

        // Verify the Seed List
        const AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();
        ASSERT_FALSE(seedList.empty());
        EXPECT_EQ(m_testDynamicSliceAssetId, seedList.at(0).m_assetId);
        EXPECT_EQ(validPlatforms, seedList.at(0).m_platformFlags);
    }

    TEST_F(AssetSeedManagerTest, AddSeedAssetForValidPlatforms_SomePlatformsValid_SeedAddedForEveryValidPlatform)
    {
        using namespace AzFramework;
        PlatformFlags validPlatforms = PlatformFlags::Platform_PC | PlatformFlags::Platform_ANDROID;
        PlatformFlags inputPlatforms = validPlatforms | PlatformFlags::Platform_MAC;
        AZStd::pair<AZ::Data::AssetId, PlatformFlags> result = m_assetSeedManager->AddSeedAssetForValidPlatforms(TestDynamicSliceAssetPath, inputPlatforms);

        // Verify the function outputs
        EXPECT_EQ(m_testDynamicSliceAssetId, result.first);
        EXPECT_EQ(validPlatforms, result.second);

        // Verify the Seed List
        const AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();
        ASSERT_FALSE(seedList.empty());
        EXPECT_EQ(m_testDynamicSliceAssetId, seedList.at(0).m_assetId);
        EXPECT_EQ(validPlatforms, seedList.at(0).m_platformFlags);
    }

    TEST_F(AssetSeedManagerTest, AddSeedAssetForValidPlatforms_NoPlatformsValid_NoSeedAdded)
    {
        using namespace AzFramework;
        PlatformFlags inputPlatforms = PlatformFlags::Platform_MAC;
        AZStd::pair<AZ::Data::AssetId, PlatformFlags> result = m_assetSeedManager->AddSeedAssetForValidPlatforms(TestDynamicSliceAssetPath, inputPlatforms);

        // Verify the function outputs
        EXPECT_FALSE(result.first.IsValid());
        EXPECT_EQ(PlatformFlags::Platform_NONE, result.second);

        // Verify the Seed List
        const AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();
        EXPECT_TRUE(seedList.empty());
    }

    TEST_F(AssetSeedManagerTest, Valid_Seed_Remove_ForAllPlatform_OK)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);

        m_assetSeedManager->RemoveSeedAsset(assets[0].ToString<AZStd::string>(), AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);
        const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

        EXPECT_EQ(seedList.size(), 0);

        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);

        m_assetSeedManager->RemoveSeedAsset("asset0.txt", AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);
        const AzFramework::AssetSeedList& secondSeedList = m_assetSeedManager->GetAssetSeedList();
        EXPECT_EQ(secondSeedList.size(), 0);
    }

    TEST_F(AssetSeedManagerTest, Valid_Seed_Remove_ForSpecificPlatform_OK)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);

        m_assetSeedManager->RemoveSeedAsset(assets[0].ToString<AZStd::string>(), AzFramework::PlatformFlags::Platform_MAC);
        const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

        EXPECT_EQ(seedList.size(), 1);

        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);

        m_assetSeedManager->RemoveSeedAsset("asset0.txt", AzFramework::PlatformFlags::Platform_PC);
        const AzFramework::AssetSeedList& secondSeedList = m_assetSeedManager->GetAssetSeedList();
        EXPECT_EQ(secondSeedList.size(), 1);
    }

    TEST_F(AssetSeedManagerTest, Invalid_NotRemove_SeedForAllPlatform_Ok)
    {
        m_assetSeedManager->AddSeedAsset(assets[0], AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);

        m_assetSeedManager->RemoveSeedAsset(assets[1].ToString<AZStd::string>(), AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);
        const AzFramework::AssetSeedList& seedList = m_assetSeedManager->GetAssetSeedList();

        EXPECT_EQ(seedList.size(), 1);

        m_assetSeedManager->RemoveSeedAsset("asset1.txt", AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_MAC);
        const AzFramework::AssetSeedList& secondSeedList = m_assetSeedManager->GetAssetSeedList();
        EXPECT_EQ(secondSeedList.size(), 1);
    }
}

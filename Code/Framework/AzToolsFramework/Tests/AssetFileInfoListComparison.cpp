/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace // anonymous
{
    constexpr int TotalAssets = 6;
    constexpr int TotalTempFiles = 3;
    constexpr char TempFiles[TotalTempFiles][AZ_MAX_PATH_LEN] = { "firstAssetFileInfoList.assetlist", "secondAssetFileInfoList.assetlist", "assetFileInfoList.assetlist" };

    enum FileIndex
    {
        FirstAssetFileInfoList,
        SecondAssetFileInfoList,
        ResultAssetFileInfoList
    };
}


namespace UnitTest
{
    class AssetFileInfoListComparisonTest
        : public LeakDetectionFixture
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
            auto projectPathOverride = FixedValueString::format(R"(--project-path="%s")", m_tempDir.GetDirectory());
            argContainer.push_back(projectPathOverride.data());
            m_application = new ToolsTestApplication("AssetFileInfoListComparisonTest", aznumeric_caster(argContainer.size()), argContainer.data());
            AzToolsFramework::AssetSeedManager assetSeedManager;
            AzFramework::AssetRegistry assetRegistry;

            const AZ::PlatformId thisPlatform = AZ::PlatformHelper::GetPlatformIdFromName(AZ::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME));
            const AZStd::string assetRoot = AzToolsFramework::PlatformAddressedAssetCatalog::GetAssetRootForPlatform(thisPlatform);

            for (int idx = 0; idx < TotalAssets; idx++)
            {
                m_assets[idx] = AssetId(AZ::Uuid::CreateRandom(), 0);
                AZ::Data::AssetInfo info;
                info.m_relativePath = AZStd::string::format("Asset%d.txt", idx);
                info.m_assetId = m_assets[idx];
                assetRegistry.RegisterAsset(m_assets[idx], info);

                AzFramework::StringFunc::Path::Join(assetRoot.c_str(), info.m_relativePath.c_str(), m_assetsPath[idx]);
                AZ_TEST_START_TRACE_SUPPRESSION;
                if (m_fileStreams[idx].Open(m_assetsPath[idx].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
                {
                    AZ::IO::SizeType bytesWritten = m_fileStreams[idx].Write(info.m_relativePath.size(), info.m_relativePath.data());
                    EXPECT_EQ(bytesWritten, info.m_relativePath.size());
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
                }
                else
                {
                    GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to create temporary file ( %s ) in AssetSeedManager unit tests.\n", m_assetsPath[idx].c_str()).c_str());
                }
            }

            // asset1 -> asset2
            assetRegistry.RegisterAssetDependency(m_assets[1], AZ::Data::ProductDependency(m_assets[2], 0));
            // asset2 -> asset3
            assetRegistry.RegisterAssetDependency(m_assets[2], AZ::Data::ProductDependency(m_assets[3], 0));
            // asset3 -> asset4
            assetRegistry.RegisterAssetDependency(m_assets[3], AZ::Data::ProductDependency(m_assets[4], 0));

        AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_application->Start(AzFramework::Application::Descriptor(), startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            ASSERT_TRUE(context) << "No serialize context.\n";

            AzToolsFramework::AssetSeedManager::Reflect(context);

            // Asset Catalog does not expose its internal asset registry and the only way to set it is through LoadCatalog API
            // Currently I am serializing the asset registry to disk
            // and invoking the LoadCatalog API to populate the asset catalog created by the azframework app.

            const AZStd::string catalogFile = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(thisPlatform);

            bool catalogSaved = AzFramework::AssetCatalog::SaveCatalog(catalogFile.c_str(), &assetRegistry);
            EXPECT_TRUE(catalogSaved) << "Unable to save the asset catalog file.\n";

            m_catalog = new AzToolsFramework::PlatformAddressedAssetCatalog(thisPlatform);

            const auto thisPlatformFlags = AZ::PlatformHelper::GetPlatformFlag(AZ::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME));
            assetSeedManager.AddSeedAsset(m_assets[0], thisPlatformFlags);
            assetSeedManager.AddSeedAsset(m_assets[1], thisPlatformFlags);

            bool firstAssetFileInfoListSaved = assetSeedManager.SaveAssetFileInfo(TempFiles[FileIndex::FirstAssetFileInfoList], thisPlatformFlags, {});
            EXPECT_TRUE(firstAssetFileInfoListSaved);

            // Modify contents of asset2
            int fileIndex = 2;
            AZ_TEST_START_TRACE_SUPPRESSION;
            if (m_fileStreams[fileIndex].Open(m_assetsPath[fileIndex].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
            {
                AZStd::string fileContent = AZStd::string::format("new Asset%d.txt", fileIndex);// changing file content
                AZ::IO::SizeType bytesWritten = m_fileStreams[fileIndex].Write(fileContent.size(), fileContent.c_str());
                EXPECT_EQ(bytesWritten, fileContent.size());
                AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
            }
            else
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to open asset file.\n").c_str());
            }

            // Modify contents of asset 4
            fileIndex = 4;
            AZ_TEST_START_TRACE_SUPPRESSION;
            if (m_fileStreams[fileIndex].Open(m_assetsPath[fileIndex].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
            {
                AZStd::string fileContent = AZStd::string::format("new Asset%d.txt", fileIndex);// changing file content
                AZ::IO::SizeType bytesWritten = m_fileStreams[fileIndex].Write(fileContent.size(), fileContent.c_str());
                EXPECT_EQ(bytesWritten, fileContent.size());
                AZ_TEST_STOP_TRACE_SUPPRESSION(1); // writing to asset cache folder
            }
            else
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to open asset file.\n").c_str());
            }

            assetSeedManager.RemoveSeedAsset(m_assets[0], thisPlatformFlags);
            assetSeedManager.AddSeedAsset(m_assets[5], thisPlatformFlags);

            bool secondAssetFileInfoListSaved = assetSeedManager.SaveAssetFileInfo(TempFiles[FileIndex::SecondAssetFileInfoList], thisPlatformFlags, {});
            EXPECT_TRUE(secondAssetFileInfoListSaved);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

            // Delete all temporary files
            for (int idx = 0; idx < TotalTempFiles; idx++)
            {
                if (fileIO->Exists(TempFiles[idx]))
                {
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    AZ::IO::Result result = fileIO->Remove(TempFiles[idx]);
                    EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // deleting from asset cache folder
                }
            }

            // Deleting all temporary assets files
            for (int idx = 0; idx < TotalAssets; idx++)
            {
                // we need to close the handle before we try to remove the file
                m_fileStreams[idx].Close();
                if (fileIO->Exists(m_assetsPath[idx].c_str()))
                {
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    AZ::IO::Result result = fileIO->Remove(m_assetsPath[idx].c_str());
                    EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // deleting from asset cache folder
                }
            }

            auto pcCatalogFile = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId::PC);
            if (fileIO->Exists(pcCatalogFile.c_str()))
            {
                AZ_TEST_START_TRACE_SUPPRESSION;
                AZ::IO::Result result = fileIO->Remove(pcCatalogFile.c_str());
                EXPECT_EQ(result.GetResultCode(), AZ::IO::ResultCode::Success);
                AZ_TEST_STOP_TRACE_SUPPRESSION(1); // deleting from asset cache folder
            }

            delete m_catalog;
            m_application->Stop();
            delete m_application;

        }

        void AssetFileInfoValidation_DeltaComparison_Valid()
        {
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AzToolsFramework::AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData comparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta, TempFiles[FileIndex::ResultAssetFileInfoList]);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            comparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);

            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Delta operation failed.\n";

            // AssetFileInfo should contain {2*, 4*, 5}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;

            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 3);

            // Verifying that the hash of the file are correct. They must be from the second AssetFileInfoList.
            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[2], m_assets[4], m_assets[5] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }


        void AssetFileInfoValidation_UnionComparison_Valid()
        {
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AzToolsFramework::AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData comparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Union, TempFiles[FileIndex::ResultAssetFileInfoList]);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            comparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Union operation failed.\n";

            // AssetFileInfo should contain {0, 1, 2*, 3, 4*, 5}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 6);

            //Verifying that the hash of the files are correct.
            AzToolsFramework::AssetFileInfoList firstAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::FirstAssetFileInfoList], firstAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> firstAssetIdToAssetFileInfoMap;

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
            {
                firstAssetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> secondAssetIdToAssetFileInfoMap;

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                secondAssetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto foundFirst = firstAssetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                auto foundSecond = secondAssetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (foundSecond != secondAssetIdToAssetFileInfoMap.end())
                {
                    // Even if the asset Id is present in both the AssetFileInfo List, it should match the file hash from the second AssetFileInfo list
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (foundSecond->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
                else if (foundFirst != firstAssetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (foundFirst->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
                else
                {
                    GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[0], m_assets[1], m_assets[2], m_assets[3], m_assets[4], m_assets[5] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }

        void AssetFileInfoValidation_IntersectionComparison_Valid()
        {
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AzToolsFramework::AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData comparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Intersection, TempFiles[FileIndex::ResultAssetFileInfoList]);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            comparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Intersection operation failed.\n";

            // AssetFileInfo should contain {1,2*,3,4*}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 4);

            // Verifying that the hash of the file are correct. They must be from the second AssetFileInfoList.
            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;

            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[1], m_assets[2], m_assets[3], m_assets[4] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }

        void AssetFileInfoValidation_ComplementComparison_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData comparisonData(AssetFileInfoListComparison::ComparisonType::Complement, TempFiles[FileIndex::ResultAssetFileInfoList]);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            comparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Complement comparison failed.\n";

            // AssetFileInfo should contain {5}
            AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";
            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 1);

            // Verifying that the hash of the file are correct. They must be from the second AssetFileInfoList.
            AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            AZStd::unordered_map<AZ::Data::AssetId, AssetFileInfo> assetIdToAssetFileInfoMap;

            for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[5] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }

        void AssetFileInfoValidation_FilePatternWildcardComparisonAll_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AssetFileInfoListComparison::ComparisonData comparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern, TempFiles[FileIndex::ResultAssetFileInfoList], "Asset*.txt", AssetFileInfoListComparison::FilePatternType::Wildcard);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "File pattern match failed.\n";

            // AssetFileInfo should contain {0,1,2,3,4}
            AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 5);

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[0], m_assets[1], m_assets[2], m_assets[3], m_assets[4] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }

        void AssetFileInfoValidation_FilePatternWildcardComparisonNone_ExpectFailure()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AssetFileInfoListComparison::ComparisonData comparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern, TempFiles[FileIndex::ResultAssetFileInfoList], "Foo*.txt", AssetFileInfoListComparison::FilePatternType::Wildcard);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_FALSE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "File pattern match should not have produced any output.\n";

            // AssetFileInfo should not exist on-disk
            ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(TempFiles[FileIndex::ResultAssetFileInfoList])) << "Asset List file should not exist on-disk.\n";
        }

        void AssetFileInfoValidation_FilePatternRegexComparisonPartial_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AssetFileInfoListComparison::ComparisonData comparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern, TempFiles[FileIndex::ResultAssetFileInfoList], "Asset[0-3].txt", AssetFileInfoListComparison::FilePatternType::Regex);
            comparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(comparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "File pattern match failed.\n";

            // AssetFileInfo should be {0,1,2,3}
            AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 4);

            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[0], m_assets[1], m_assets[2], m_assets[3]};

            for (const AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);
        }

        void AssetFileInfoValidation_DeltaFilePatternComparisonOperation_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData deltaComparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta, "$1");
            deltaComparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            deltaComparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(deltaComparisonData);

            AssetFileInfoListComparison::ComparisonData filePatternComparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern, TempFiles[FileIndex::ResultAssetFileInfoList], "Asset[0-3].txt", AssetFileInfoListComparison::FilePatternType::Regex);
            filePatternComparisonData.m_firstInput = "$1";
            assetFileInfoListComparison.AddComparisonStep(filePatternComparisonData);
            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Multiple Comparison Operation( Delta + FilePattern ) failed.\n";

            // Output of the Delta Operation should be {2*, 4*, 5}
            // Output of the FilePattern Operation should be {2*}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 1);

            AZStd::unordered_map<AZ::Data::AssetId, AssetFileInfo> assetIdToAssetFileInfoMap;

            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[2] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);

        }

        void AssetFileInfoValidation_FilePatternDeltaComparisonOperation_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AssetFileInfoListComparison::ComparisonData filePatternComparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern,"$1", "Asset[0-3].txt", AssetFileInfoListComparison::FilePatternType::Regex);
            filePatternComparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(filePatternComparisonData);

            AzToolsFramework::AssetFileInfoListComparison::ComparisonData deltaComparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta, TempFiles[FileIndex::ResultAssetFileInfoList]);
            deltaComparisonData.m_firstInput = "$1";
            deltaComparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(deltaComparisonData);

            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Multiple Comparison Operation( FilePattern + Delta ) failed.\n";
            // Output of the FilePattern Operation should be {0,1,2,3}
            // Output of the Delta Operation should be {2*,4*,5}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 3);

            AZStd::unordered_map<AZ::Data::AssetId, AssetFileInfo> assetIdToAssetFileInfoMap;

            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[2], m_assets[4], m_assets[5] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);

        }

        void AssetFileInfoValidation_DeltaUnionFilePatternComparisonOperation_Valid()
        {
            using namespace AzToolsFramework;
            // First AssetFileInfoList {0,1,2,3,4} , Second AssetFileInfoList {1,2*,3,4*,5} where * indicate that hash has changed for that asset
            AssetFileInfoListComparison assetFileInfoListComparison;
            AzToolsFramework::AssetFileInfoListComparison::ComparisonData deltaComparisonData(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta, "$1");
            deltaComparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            deltaComparisonData.m_secondInput = TempFiles[FileIndex::SecondAssetFileInfoList];
            assetFileInfoListComparison.AddComparisonStep(deltaComparisonData);

            AssetFileInfoListComparison::ComparisonData unionComparisonData(AssetFileInfoListComparison::ComparisonType::Union, "$2");
            unionComparisonData.m_firstInput = TempFiles[FileIndex::FirstAssetFileInfoList];
            unionComparisonData.m_secondInput = "$1";
            assetFileInfoListComparison.AddComparisonStep(unionComparisonData);

            AssetFileInfoListComparison::ComparisonData filePatternComparisonData(AssetFileInfoListComparison::ComparisonType::FilePattern, TempFiles[FileIndex::ResultAssetFileInfoList], "Asset[4-5].txt", AssetFileInfoListComparison::FilePatternType::Regex);
            filePatternComparisonData.m_firstInput = "$2";
            assetFileInfoListComparison.AddComparisonStep(filePatternComparisonData);

            ASSERT_TRUE(assetFileInfoListComparison.CompareAndSaveResults().IsSuccess()) << "Multiple Comparison Operation( Delta + Union + FilePattern ) failed.\n";

            // Output of the Delta Operation should be {2*, 4*, 5}
            // Putput of the Union Operation should be {0, 1, 2*, 3, 4*, 5}
            // Output of the FilePattern Operation should be {4*, 5}
            AzToolsFramework::AssetFileInfoList assetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::ResultAssetFileInfoList], assetFileInfoList)) << "Unable to read the asset file info list.\n";

            EXPECT_EQ(assetFileInfoList.m_fileInfoList.size(), 2);

            AZStd::unordered_map<AZ::Data::AssetId, AssetFileInfo> assetIdToAssetFileInfoMap;

            AzToolsFramework::AssetFileInfoList secondAssetFileInfoList;
            ASSERT_TRUE(AZ::Utils::LoadObjectFromFileInPlace(TempFiles[FileIndex::SecondAssetFileInfoList], secondAssetFileInfoList)) << "Unable to read the asset file info list.\n";

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = AZStd::move(assetFileInfo);
            }

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
                if (found != assetIdToAssetFileInfoMap.end())
                {
                    // checking the file hash
                    for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                    {
                        if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                        {
                            GTEST_FATAL_FAILURE_(AZStd::string::format("Invalid file hash.\n").c_str());
                            break;
                        }
                    }
                }
            }

            // Verifying that correct assetId are present in the assetFileInfo list
            AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds{ m_assets[4], m_assets[5] };

            for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
            {
                auto found = expectedAssetIds.find(assetFileInfo.m_assetId);
                if (found != expectedAssetIds.end())
                {
                    expectedAssetIds.erase(found);
                }
            }

            EXPECT_EQ(expectedAssetIds.size(), 0);

        }

        ToolsTestApplication* m_application = nullptr;
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
        AzToolsFramework::PlatformAddressedAssetCatalog* m_catalog = nullptr;
        AZ::IO::FileIOStream m_fileStreams[TotalAssets];
        AZ::Data::AssetId m_assets[TotalAssets];
        AZStd::string m_assetsPath[TotalAssets];
    };

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_DeltaComparison_Valid)
    {
        AssetFileInfoValidation_DeltaComparison_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_UnionComparison_Valid)
    {
        AssetFileInfoValidation_UnionComparison_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_IntersectionComparison_Valid)
    {
        AssetFileInfoValidation_IntersectionComparison_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_ComplementComparison_Valid)
    {
        AssetFileInfoValidation_ComplementComparison_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_FilePatternWildcardComparisonAll_Valid)
    {
        AssetFileInfoValidation_FilePatternWildcardComparisonAll_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_FilePatternWildcardComparisonNone_ExpectFailure)
    {
        AssetFileInfoValidation_FilePatternWildcardComparisonNone_ExpectFailure();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_FilePatternRegexComparisonPartial_Valid)
    {
        AssetFileInfoValidation_FilePatternRegexComparisonPartial_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_DeltaFilePatternComparisonOperation_Valid)
    {
        AssetFileInfoValidation_DeltaFilePatternComparisonOperation_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_FilePatternDeltaComparisonOperation_Valid)
    {
        AssetFileInfoValidation_FilePatternDeltaComparisonOperation_Valid();
    }

    TEST_F(AssetFileInfoListComparisonTest, AssetFileInfoValidation_DeltaUnionFilePatternComparisonOperation_Valid)
    {
        AssetFileInfoValidation_DeltaUnionFilePatternComparisonOperation_Valid();
    }
}

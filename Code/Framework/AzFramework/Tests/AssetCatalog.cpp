/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FileIOBaseTestTypes.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>
#include <AzFramework/Application/Application.h>

namespace UnitTest
{
    namespace
    {
        AZ::Data::AssetId asset1;
        AZ::Data::AssetId asset2;
        AZ::Data::AssetId asset3;
        AZ::Data::AssetId asset4;
        AZ::Data::AssetId asset5;

        bool Search(const AZStd::vector<AZ::Data::ProductDependency>& dependencies, const AZ::Data::AssetId& assetId)
        {
            for (const auto& dependency : dependencies)
            {
                if (dependency.m_assetId == assetId)
                {
                    return true;
                }
            }

            return false;
        }
    }

    class AssetCatalogDependencyTest
        : public LeakDetectionFixture
    {
        AzFramework::AssetCatalog* m_assetCatalog;

    public:

        const char* path1 = "SomeFolder/Asset1Path";
        const char* path2 = "SomeFolder/Asset2Path";
        const char* path3 = "OtherFolder/Asset3Path";
        const char* path4 = "OtherFolder/Asset4Path";
        const char* path5 = "OtherFolder/Asset5Path";

        void SetUp() override
        {
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            m_assetCatalog = aznew AzFramework::AssetCatalog();
            m_assetCatalog->StartMonitoringAssets();

            AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
            ASSERT_NE(notificationInterface, nullptr);

            asset1 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset2 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset3 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset4 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset5 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);

            AZ::Data::AssetInfo info1, info2, info3, info4, info5;
            info1.m_relativePath = path1;
            info1.m_sizeBytes = 1; // Need to initialize m_sizeBytes to non-zero number to avoid a FileIO call
            info2.m_relativePath = path2;
            info2.m_sizeBytes = 1;
            info3.m_relativePath = path3;
            info3.m_sizeBytes = 1;
            info4.m_relativePath = path4;
            info4.m_sizeBytes = 1;
            info5.m_relativePath = path5;
            info5.m_sizeBytes = 1;

            // asset1 -> asset2 -> asset3 -> asset5
            //                 --> asset4

            {
                m_assetCatalog->RegisterAsset(asset1, info1);
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset1;
                message.m_dependencies.emplace_back(asset2, 0);
                notificationInterface->AssetChanged({ message });
            }

            {
                m_assetCatalog->RegisterAsset(asset2, info2);
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset2;
                message.m_dependencies.emplace_back(asset3, 0);
                message.m_dependencies.emplace_back(asset4, 0);
                notificationInterface->AssetChanged({ message });
            }

            {
                m_assetCatalog->RegisterAsset(asset3, info3);
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset3;
                message.m_dependencies.emplace_back(asset5, 0);
                notificationInterface->AssetChanged({ message });
            }

            {
                m_assetCatalog->RegisterAsset(asset4, info4);
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset4;
                notificationInterface->AssetChanged({ message });
            }

            {
                m_assetCatalog->RegisterAsset(asset5, info5);
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset5;
                notificationInterface->AssetChanged({ message });
            }

        }

        void TearDown() override
        {
            delete m_assetCatalog;

            AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
            AZ::TickBus::ClearQueuedEvents();

            AZ::Data::AssetManager::Destroy();
        }

        void CheckDirectDependencies(AZ::Data::AssetId assetId, AZStd::initializer_list<AZ::Data::AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }

        void CheckAllDependencies(AZ::Data::AssetId assetId, AZStd::initializer_list<AZ::Data::AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependencies, assetId);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }

        void CheckAllDependenciesFilter(AZ::Data::AssetId assetId, const AZStd::unordered_set<AZ::Data::AssetId>& filterSet, const AZStd::vector<AZStd::string>& wildcardList, AZStd::initializer_list<AZ::Data::AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependenciesFilter, assetId, filterSet, wildcardList);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }
    };

    TEST_F(AssetCatalogDependencyTest, directDependencies)
    {
        CheckDirectDependencies(asset1, { asset2 });
        CheckDirectDependencies(asset2, { asset3, asset4 });
        CheckDirectDependencies(asset3, { asset5 });
        CheckDirectDependencies(asset4, { });
        CheckDirectDependencies(asset5, { });
    }

    TEST_F(AssetCatalogDependencyTest, allDependencies)
    {
        CheckAllDependencies(asset1, { asset2, asset3, asset4, asset5 });
        CheckAllDependencies(asset2, { asset3, asset4, asset5 });
        CheckAllDependencies(asset3, { asset5 });
        CheckAllDependencies(asset4, {});
        CheckAllDependencies(asset5, {});
    }

    TEST_F(AssetCatalogDependencyTest, allDependenciesFilter_AssetIdFilter)
    {
        CheckAllDependenciesFilter(asset1, { asset2 }, { }, { });
        CheckAllDependenciesFilter(asset1, { asset3 }, { }, { asset2 });
        CheckAllDependenciesFilter(asset1, { asset5 }, { }, { asset2, asset3, asset4 });
        CheckAllDependenciesFilter(asset1, { asset4, asset5 }, { }, { asset2, asset3 });
    }

    TEST_F(AssetCatalogDependencyTest, allDependenciesFilter_WildcardFilter)
    {
        CheckAllDependenciesFilter(asset1, { }, { "*/*" }, { });
        CheckAllDependenciesFilter(asset1, { }, { "*/Asset?Path" }, {});
        CheckAllDependenciesFilter(asset1, { }, { "other*" }, { asset2 });
        CheckAllDependenciesFilter(asset1, { }, { "*4*" }, { asset2, asset3, asset5 });
        CheckAllDependenciesFilter(asset1, { }, { "*4*", "*5*" }, { asset2, asset3 });
    }

    TEST_F(AssetCatalogDependencyTest, allDependenciesFilter_AssetIdAndWildcardFilter)
    {
        CheckAllDependenciesFilter(asset1, { asset5 }, { "*4*" }, { asset2, asset3 });
    }

    TEST_F(AssetCatalogDependencyTest, unregisterTest)
    {
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset4);
        CheckDirectDependencies(asset2, { asset3 });
        CheckAllDependencies(asset1, { asset2, asset3, asset5 });
    }

    class AssetCatalogDeltaTest :
        public LeakDetectionFixture
    {
    public:
        const char* path1 = "Asset1Path";
        const char* path2 = "Asset2Path";
        const char* path3 = "Asset3Path";
        const char* path4 = "Asset4Path";
        const char* path5 = "Asset5Path";
        const char* path6 = "Asset6Path";

        AZ::IO::FixedMaxPath sourceCatalogPath1;
        AZ::IO::FixedMaxPath sourceCatalogPath2;
        AZ::IO::FixedMaxPath sourceCatalogPath3;

        AZ::IO::FixedMaxPath baseCatalogPath;
        AZ::IO::FixedMaxPath deltaCatalogPath;
        AZ::IO::FixedMaxPath deltaCatalogPath2;
        AZ::IO::FixedMaxPath deltaCatalogPath3;

        AZStd::vector<AZStd::string> deltaCatalogFiles;
        AZStd::vector<AZStd::string> deltaCatalogFiles2;
        AZStd::vector<AZStd::string> deltaCatalogFiles3;

        AZStd::shared_ptr<AzFramework::AssetRegistry> baseCatalog;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog2;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog3;

        AZStd::unique_ptr<AzFramework::Application> m_app;
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;

        void SetUp() override
        {
            m_app.reset(aznew AzFramework::Application());
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;

            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadAssetCatalog = false;
            startupParameters.m_loadSettingsRegistry = false;
            m_app->Start(desc, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            asset1 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset2 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset3 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset4 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            asset5 = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);

            sourceCatalogPath1 = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogSource1.xml";
            sourceCatalogPath2 = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogSource2.xml";
            sourceCatalogPath3 = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogSource3.xml";

            baseCatalogPath = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogBase.xml";
            deltaCatalogPath = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogDelta.xml";
            deltaCatalogPath2 = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogDelta2.xml";
            deltaCatalogPath3 = m_tempDirectory.GetDirectoryAsPath() / "AssetCatalogDelta3.xml";

            AZ::Data::AssetInfo info1, info2, info3, info4, info5, info6;

            info1.m_relativePath = path1;
            info2.m_relativePath = path2;
            info3.m_relativePath = path3;
            info4.m_relativePath = path4;
            info5.m_relativePath = path5;
            info6.m_relativePath = path6;

            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset1, info1);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            // baseCatalog - asset1 path1, asset2 path2
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, baseCatalogPath.c_str());

            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset1, info3);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset4, info4);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StartMonitoringAssets);
            AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
            ASSERT_NE(notificationInterface, nullptr);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path3, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset1;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                notificationInterface->AssetChanged({ message });
            }
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StopMonitoringAssets);
            // sourcecatalog1 - asset1 path3 (depends on asset 2), asset2 path2, asset4 path4
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath1.c_str());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            // deltacatalog - asset1 path3 (depends on asset 2), asset4 path4
            deltaCatalogFiles.push_back(path3);
            deltaCatalogFiles.push_back(path4);

            // deltacatalog - asset1 path3, asset4 path4
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath.c_str());

            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset5, info5);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StartMonitoringAssets);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path5, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset5;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                notificationInterface->AssetChanged({ message });
            }
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StopMonitoringAssets);
            // sourcecatalog2 - asset1 path3 (depends on asset 2), asset2 path2, asset4 path4, asset5 path5 (depends on asset 2)
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath2.c_str());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset1);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset4);
            // deltacatalog2 - asset5 path5 (depends on asset 2)
            deltaCatalogFiles2.push_back(path5);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath2.c_str());

            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset1, info6);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, asset5, info4);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StartMonitoringAssets);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path4, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
                message.m_assetId = asset5;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                notificationInterface->AssetChanged({ message });
            }
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StopMonitoringAssets);
            //sourcecatalog3 - asset1 path6, asset2 path2, asset5 path4 (depends on asset 2)
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath3.c_str());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            // deltacatalog3 - asset1 path6 asset5 path4 (depends on asset 2)
            deltaCatalogFiles3.push_back(path6);
            deltaCatalogFiles3.push_back(path4);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath3.c_str());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset1);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::UnregisterAsset, asset5);

            baseCatalog = AzFramework::AssetCatalog::LoadCatalogFromFile(baseCatalogPath.c_str());
            deltaCatalog = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath.c_str());
            deltaCatalog2 = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath2.c_str());
            deltaCatalog3 = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath3.c_str());
        }

        void TearDown() override
        {
            deltaCatalogFiles.set_capacity(0);
            deltaCatalogFiles2.set_capacity(0);
            deltaCatalogFiles3.set_capacity(0);
            baseCatalog.reset();
            deltaCatalog.reset();
            deltaCatalog2.reset();
            deltaCatalog3.reset();
            m_app->Stop();
            m_app.reset();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GarbageCollect();
        }

        void CheckDirectDependencies(AZ::Data::AssetId assetId, AZStd::initializer_list<AZ::Data::AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }

        void CheckNoDependencies(AZ::Data::AssetId assetId)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);
            EXPECT_FALSE(result.IsSuccess());
        }
    };

    TEST_F(AssetCatalogDeltaTest, LoadCatalog_AssetChangedCatalogLoaded_AssetStillKnown)
    {
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::StartMonitoringAssets);
        auto updatedAsset = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
        AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
        ASSERT_NE(notificationInterface, nullptr);
        {
            AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom(), "");
            message.m_assetId = updatedAsset;
            message.m_dependencies.emplace_back(asset2, 0);
            notificationInterface->AssetChanged({ message });
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, updatedAsset);
        EXPECT_TRUE(assetInfo.m_assetId.IsValid());

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, baseCatalogPath.c_str());

        //Asset should still be known - the catalog should swap in the new catalog from disk
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, updatedAsset);
        EXPECT_TRUE(assetInfo.m_assetId.IsValid());
    }

    TEST_F(AssetCatalogDeltaTest, DeltaCatalogTest)
    {
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, "");

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, baseCatalogPath.c_str());

        // Topmost should have precedence

        // baseCatalog path1 path2
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        // deltacatalog path3                path4
        /// baseCatalog path1  path2
        ///             asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog2);

        // deltacatalog2                             path5
        //  deltacatalog path3                path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog);

        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::InsertDeltaCatalog, deltaCatalog3, 0);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        // deltacatalog3 path6                       path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog3);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::InsertDeltaCatalog, deltaCatalog3, 1);

        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, baseCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::InsertDeltaCatalog, baseCatalog, 3);

        ///  baseCatalog path1  path2
        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });
    }

    TEST_F(AssetCatalogDeltaTest, DeltaCatalogTest_AddDeltaCatalogNext_Success)
    {
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, "");

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, baseCatalogPath.c_str());

        // Topmost should have precedence

        // baseCatalog path1 path2
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        // deltacatalog path3                path4
        /// baseCatalog path1  path2
        ///             asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog2);

        // deltacatalog2                             path5
        //  deltacatalog path3                path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog);

        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::InsertDeltaCatalogBefore, deltaCatalog3, deltaCatalog2);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        // deltacatalog3 path6                       path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog3);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::InsertDeltaCatalogBefore, deltaCatalog3, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RemoveDeltaCatalog, baseCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::AddDeltaCatalog, baseCatalog);

        ///  baseCatalog path1  path2
        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///              asset1 asset2 asset3 asset4 asset5
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });
    }

    TEST_F(AssetCatalogDeltaTest, DeltaCatalogCreationTest)
    {
        bool result = false;
        AZStd::string assetPath;

        // load source catalog 1
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath1.c_str());
        // generate delta catalog 1
        AZ::IO::Path testDeltaCatalogPath1 = m_tempDirectory.GetDirectoryAsPath() / "TestAssetCatalogDelta.xml";
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles, testDeltaCatalogPath1.Native());
        EXPECT_TRUE(result);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath1.c_str());
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset4);
        EXPECT_EQ(assetPath, path4);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against depenency info
        CheckDirectDependencies(asset1, { asset2 });


        // load source catalog 2
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath2.c_str());
        // generate delta catalog 3
        AZ::IO::Path testDeltaCatalogPath2 = m_tempDirectory.GetDirectoryAsPath() / "TestAssetCatalogDelta2.xml";
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles2, testDeltaCatalogPath2.Native());
        EXPECT_TRUE(result);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath2.c_str());
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against dependency info
        CheckDirectDependencies(asset5, { asset2 });


        // load source catalog 3
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath3.c_str());
        // generate delta catalog 3
        AZ::IO::Path testDeltaCatalogPath3 = m_tempDirectory.GetDirectoryAsPath() / "TestAssetCatalogDelta3.xml";
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles3, testDeltaCatalogPath3.Native());
        EXPECT_TRUE(result);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::ClearCatalog);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath3.c_str());
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path6);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against dependency info
        CheckDirectDependencies(asset5, { asset2 });
        CheckNoDependencies(asset1);
    }

    class AssetCatalogAPITest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            using namespace AZ::Data;
            m_application = new AzFramework::Application();
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            ASSERT_NE(nullptr, settingsRegistry);
            EXPECT_TRUE(settingsRegistry->Get(m_assetRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder));
            m_assetCatalog = aznew AzFramework::AssetCatalog();

            m_firstAssetId = AssetId(AZ::Uuid::CreateRandom(), 0);
            AssetInfo assetInfo;
            assetInfo.m_assetId = m_firstAssetId;
            assetInfo.m_relativePath = "AssetA.txt";
            assetInfo.m_sizeBytes = 1; //any non zero value
            m_assetCatalog->RegisterAsset(m_firstAssetId, assetInfo);

            m_secondAssetId = AssetId(AZ::Uuid::CreateRandom(), 0);
            assetInfo.m_assetId = m_secondAssetId;
            assetInfo.m_relativePath = "Foo/AssetA.txt";
            assetInfo.m_sizeBytes = 1; //any non zero value
            m_assetCatalog->RegisterAsset(m_secondAssetId, assetInfo);
        }

        void TearDown() override
        {
            delete m_assetCatalog;
            delete m_application;
        }

        AzFramework::Application* m_application;
        AzFramework::AssetCatalog* m_assetCatalog;
        AZ::Data::AssetId m_firstAssetId;
        AZ::Data::AssetId m_secondAssetId;
        AZ::IO::Path m_assetRoot;
    };

    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsolutePath_Valid)
    {
        AZ::IO::Path absPath = m_assetRoot / "AssetA.txt";
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath(absPath.c_str(), AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsolutePathWithSubFolder_Valid)
    {
        AZ::IO::Path absPath = (m_assetRoot / "Foo/AssetA.txt").LexicallyNormal();
        EXPECT_EQ(m_secondAssetId, m_assetCatalog->GetAssetIdByPath(absPath.c_str(), AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_RelPath_Valid)
    {
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath("AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_RelPathWithSubFolders_Valid)
    {
        EXPECT_EQ(m_secondAssetId, m_assetCatalog->GetAssetIdByPath("Foo/AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsPath_Invalid)
    {
        // A path that begins with a Posix path separator '/' is absolute not a relative path
        EXPECT_NE(m_firstAssetId, m_assetCatalog->GetAssetIdByPath("//AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetRegisteredAssetPaths_TwoAssetsRegistered_RetrieveAssetPaths)
    {
        AZStd::vector<AZStd::string> assetPaths = m_assetCatalog->GetRegisteredAssetPaths();
        EXPECT_EQ(assetPaths.size(), 2);
        ASSERT_THAT(assetPaths, testing::ElementsAre(
            "AssetA.txt",
            "Foo/AssetA.txt"));
    }

    TEST_F(AssetCatalogAPITest, DoesAssetIdMatchWildcardPattern_Matches)
    {
        EXPECT_TRUE(m_assetCatalog->DoesAssetIdMatchWildcardPattern(m_firstAssetId, "*.txt"));
        EXPECT_TRUE(m_assetCatalog->DoesAssetIdMatchWildcardPattern(m_firstAssetId, "asset?.txt"));
    }

    TEST_F(AssetCatalogAPITest, DoesAssetIdMatchWildcardPattern_DoesNotMatch)
    {
        EXPECT_FALSE(m_assetCatalog->DoesAssetIdMatchWildcardPattern(AZ::Data::AssetId(), "*.txt"));
        EXPECT_FALSE(m_assetCatalog->DoesAssetIdMatchWildcardPattern(m_firstAssetId, "*.xml"));
        EXPECT_FALSE(m_assetCatalog->DoesAssetIdMatchWildcardPattern(m_firstAssetId, ""));
    }

    TEST_F(AssetCatalogAPITest, EnumerateAssetsListsCorrectAssets)
    {
        AZStd::vector<AZ::Data::AssetId> foundAssets;

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr,
            [&foundAssets](const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
            {
                foundAssets.emplace_back(assetId);
            },
            nullptr);

        ASSERT_EQ(foundAssets.size(), 2);
        EXPECT_EQ(foundAssets[0], m_firstAssetId);
        EXPECT_EQ(foundAssets[1], m_secondAssetId);
    }

    TEST_F(AssetCatalogAPITest, EnumerateAssetsDoesNotBlockMutex)
    {
        // This test simulates a previously-occurring deadlock bug caused by having the main thread call AssetCatalog::EnumerateAssets
        // with a callback that calls the AssetManager, and a loading thread running underneath the AssetManager that calls the
        // AssetCatalog.
        // To reproduce this state, we will do the following:
        // Main Thread                  Job Thread
        //                              wait on mainToJobSync
        // call EnumerateAssets
        // unblock mainToJobSync
        // wait on jobToMainSync        call AssetCatalog::X
        //                              unblock jobToMainSync
        //
        // These steps should ensure that we create a theoretical "lock inversion", if EnumerateAssets locked an AssetCatalog mutex.
        // Even though we're using binary semaphores instead of mutexes, we're creating the same blocking situation.
        // EnumerateAssets itself should no longer lock the AssetCatalog mutex, so this test should succeed since there won't be any
        // actual lock inversion.

        // Set up job manager with one thread that we can use to set up the concurrent mutex access.
        AZ::JobManagerDesc desc;
        AZ::JobManagerThreadDesc threadDesc;
        desc.m_workerThreads.push_back(threadDesc);
        auto jobManager = aznew AZ::JobManager(desc);
        auto jobContext = aznew AZ::JobContext(*jobManager);
        AZ::JobContext::SetGlobalContext(jobContext);

        AZStd::binary_semaphore mainToJobSync;
        AZStd::binary_semaphore jobToMainSync;

        // Create a job whose sole purpose is to wait for EnumerateAssets to get called, call AssetCatalog, then tell EnumerateAssets
        // to finish. If EnumerateAssets has a mutex held, this will deadlock.
        auto job = AZ::CreateJobFunction(
            [&mainToJobSync, &jobToMainSync]() mutable
            {
                // wait for the main thread to be inside the EnumerateAssets callback.
                mainToJobSync.acquire();

                // call AssetCatalog::X.  This will deadlock if EnumerateAssets is holding an AssetCatalog mutex.
                [[maybe_unused]] AZStd::string assetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, AZ::Data::AssetId());

                // signal the main thread to continue
                jobToMainSync.release();
            },
            true);
        job->Start();

        bool testCompletedSuccessfully = false;

        // Call EnumerateAssets with a callback that also tries to lock the mutex.
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr,
            [this, &mainToJobSync, &jobToMainSync, &testCompletedSuccessfully]
        (const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
            {
                // Only run this logic on the first assetId.
                if (assetId == m_firstAssetId)
                {
                    // Tell the job it can continue
                    mainToJobSync.release();

                    // Wait for the job to finish calling AssetCatalog::X.  This will deadlock if EnumerateAssets is holding an
                    // AssetCatalog mutex.
                    if (jobToMainSync.try_acquire_for(AZStd::chrono::seconds(5)))
                    {
                        // Only mark the test as completed successfully if we ran this logic and didn't deadlock.
                        testCompletedSuccessfully = true;
                    }

                }
            },
            nullptr);

        EXPECT_TRUE(testCompletedSuccessfully);

        // Clean up the job manager
        AZ::JobContext::SetGlobalContext(nullptr);
        delete jobContext;
        delete jobManager;
    }



    class AssetType1
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetType1, AZ::SystemAllocator);
        AZ_RTTI(AssetType1, "{64ECE3AE-2BEB-4502-9243-D17425249E08}", AZ::Data::AssetData);

        AssetType1()
            : m_data(nullptr)
        {
        }
        explicit AssetType1(const AZ::Data::AssetId& assetId, const AZ::Data::AssetData::AssetStatus assetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, assetStatus)
            , m_data(nullptr)
        {
        }
        ~AssetType1() override
        {
            if (m_data)
            {
                azfree(m_data);
            }
        }

        char* m_data;
    };

    class AssetType2
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetType2, AZ::SystemAllocator);
        AZ_RTTI(AssetType2, "{F5EDAAAB-2398-4967-A2C7-6B7C9421C1CE}", AZ::Data::AssetData);

        AssetType2()
            : m_data(nullptr)
        {
        }
        explicit AssetType2(const AZ::Data::AssetId& assetId, const AZ::Data::AssetData::AssetStatus assetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, assetStatus)
            , m_data(nullptr)
        {
        }
        ~AssetType2() override
        {
            if (m_data)
            {
                azfree(m_data);
            }
        }

        char* m_data;
    };

    class AssetCatalogDisplayNameTest
        : public LeakDetectionFixture
    {
        virtual AZ::IO::IStreamer* CreateStreamer() { return aznew AZ::IO::Streamer(AZStd::thread_desc{}, AZ::StreamerComponent::CreateStreamerStack()); }
        virtual void DestroyStreamer(AZ::IO::IStreamer* streamer) { delete streamer; }

        AZ::IO::FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;
        AZ::IO::IStreamer* m_streamer{ nullptr };
        AzFramework::AssetCatalog* m_assetCatalog;

    public:

        void SetUp() override
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);

            m_streamer = CreateStreamer();
            if (m_streamer)
            {
                AZ::Interface<AZ::IO::IStreamer>::Register(m_streamer);
            }

            // create the database
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            m_assetCatalog = aznew AzFramework::AssetCatalog();
        }

        void TearDown() override
        {
            delete m_assetCatalog;

            // destroy the database
            AZ::Data::AssetManager::Destroy();

            if (m_streamer)
            {
                AZ::Interface<AZ::IO::IStreamer>::Unregister(m_streamer);
            }
            DestroyStreamer(m_streamer);

            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }
    };

    TEST_F(AssetCatalogDisplayNameTest, GetAssetTypeByDisplayName_GetRegistered)
    {
        // Register an asset type
        AZ::TypeId assetTypeId = AZ::AzTypeInfo<AssetType1>::Uuid();
        AZStd::string assetTypeName = "Test Asset Type";
        auto handler = aznew AzFramework::GenericAssetHandler<AssetType1>(assetTypeName.data(), "SomeGroup", "someextension", assetTypeId);
        handler->Register();

        {
            // Get the Asset Type from the Display Name
            AZ::Data::AssetType id;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetTypeByDisplayName, assetTypeName);

            EXPECT_EQ(id, assetTypeId);
        }

        // Delete the handler
        handler->Unregister();
        delete handler;
    }

    TEST_F(AssetCatalogDisplayNameTest, GetAssetTypeByDisplayName_GetNotRegistered)
    {
        // Register an asset type
        AZ::TypeId assetTypeId = AZ::AzTypeInfo<AssetType1>::Uuid();
        AZStd::string assetTypeName = "Test Asset Type";
        auto handler = aznew AzFramework::GenericAssetHandler<AssetType1>(assetTypeName.data(), "SomeGroup", "someextension", assetTypeId);
        handler->Register();

        {
            // Try to get an Asset Type with a Display Name that was never registered
            AZ::Data::AssetType id;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetTypeByDisplayName, "Some Asset Type We Did Not Register");

            EXPECT_TRUE(id.IsNull());
        }

        // Delete the handler
        handler->Unregister();
        delete handler;
    }

    TEST_F(AssetCatalogDisplayNameTest, GetAssetTypeByDisplayName_GetWrongCasing)
    {
        // Register an asset type
        AZ::TypeId assetTypeId = AZ::AzTypeInfo<AssetType1>::Uuid();
        AZStd::string assetTypeName = "Test Asset Type";
        auto handler = aznew AzFramework::GenericAssetHandler<AssetType1>(assetTypeName.data(), "SomeGroup", "someextension", assetTypeId);
        handler->Register();

        {
            // Try to get the Asset Type we registered, but with a lowercase Display Name
            AZ::Data::AssetType id;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetTypeByDisplayName, "test asset type");

            EXPECT_TRUE(id.IsNull());
        }

        // Delete the handler
        handler->Unregister();
        delete handler;
    }

    TEST_F(AssetCatalogDisplayNameTest, GetAssetTypeByDisplayName_NoRegisteredType)
    {
        // Get the Asset Type from a Display Name we did not register
        AZ::Data::AssetType id;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetTypeByDisplayName, "Some Asset Type We Did Not Register");

        EXPECT_TRUE(id.IsNull());
    }

    TEST_F(AssetCatalogDisplayNameTest, GetAssetTypeByDisplayName_MultiRegisteredType_SameDisplayName)
    {
        AZStd::string assetTypeName = "Test Asset Type";

        // Register an asset type
        AZ::TypeId asset1TypeId = AZ::AzTypeInfo<AssetType1>::Uuid();
        auto handler1 = aznew AzFramework::GenericAssetHandler<AssetType1>(assetTypeName.data(), "SomeGroup", "someextension", asset1TypeId);
        handler1->Register();

        // Register a different type handler with the same Display Name
        AZ::TypeId asset2TypeId = AZ::AzTypeInfo<AssetType2>::Uuid();
        auto handler2 = aznew AzFramework::GenericAssetHandler<AssetType2>(assetTypeName.data(), "SomeGroup", "someextension", asset2TypeId);
        handler2->Register();

        // Verify one of the types is returned.
        // Note that we have no control over which one is returned as it's an implementation detail of the AssetCatalog
        {
            AZ::Data::AssetType id;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetTypeByDisplayName, assetTypeName);

            EXPECT_TRUE((id == asset1TypeId) || (id == asset2TypeId));
        }

        // Delete both handlers
        handler1->Unregister();
        handler2->Unregister();
        delete handler1;
        delete handler2;
    }
}

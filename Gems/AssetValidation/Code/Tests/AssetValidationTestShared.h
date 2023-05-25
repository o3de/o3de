/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AssetValidationSystemComponent.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSeedList.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <Tests/FileIOBaseTestTypes.h>


namespace UnitTest
{
    constexpr int NumTestAssets = 10;
    constexpr char ProjectName[] = "UnitTest";

    class MockValidationComponent
        : public AssetValidation::AssetValidationSystemComponent
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MockValidationComponent, AZ::SystemAllocator)

        MockValidationComponent()
        {
            for (int i = 0; i < NumTestAssets; ++i)
            {
                m_assetIds[i] = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            }
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
            AssetValidation::AssetValidationSystemComponent::Activate();
        }

        ~MockValidationComponent()
        {
            AssetValidation::AssetValidationSystemComponent::Deactivate();
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        void SeedMode() override
        {
            AssetValidation::AssetValidationSystemComponent::SeedMode();
        }

        bool IsKnownAsset(const char* fileName) override
        {
            return AssetValidation::AssetValidationSystemComponent::IsKnownAsset(fileName);
        }

        bool AddSeedAssetId(AZ::Data::AssetId assetId, AZ::u32 sourceId) override
        {
            return AssetValidation::AssetValidationSystemComponent::AddSeedAssetId(assetId, sourceId);
        }

        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;

            for (int slotNum = 0; slotNum < NumTestAssets; ++slotNum)
            {
                const AZ::Data::AssetId& assetId = m_assetIds[slotNum];
                if (assetId == id)
                {
                    result.m_assetId = id;
                    // Internal paths should be lower cased as from the cache
                    result.m_relativePath = AZStd::string::format("assetpath%d", slotNum);
                    break;
                }
            }
            return result;
        }

        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependencies(const AZ::Data::AssetId& id) override
        {
            AZStd::vector<AZ::Data::ProductDependency> dependencyList;
            bool foundAsset{ false };
            for (const AZ::Data::AssetId& assetId : m_assetIds)
            {
                if (assetId == id)
                {
                    foundAsset = true;
                }
                else if (foundAsset)
                {
                    AZ::Data::ProductDependency thisDependency;
                    thisDependency.m_assetId = assetId;
                    dependencyList.emplace_back(AZ::Data::ProductDependency(assetId, {}));
                }
            }
            if (dependencyList.size())
            {
                return AZ::Success(dependencyList);
            }
            return AZ::Failure(AZStd::string("Asset not found"));
        }

        bool TestAddSeedsFor(const AzFramework::AssetSeedList& seedList, AZ::u32 sourceId)
        {
            return AddSeedsFor(seedList, sourceId);
        }

        bool TestRemoveSeedsFor(const AzFramework::AssetSeedList& seedList, AZ::u32 sourceId)
        {
            return RemoveSeedsFor(seedList, sourceId);
        }

        bool TestAddSeedList(const char* seedListName)
        {
            return AddSeedList(seedListName);
        }

        bool TestRemoveSeedList(const char* seedListName)
        {
            return RemoveSeedList(seedListName);
        }

        AZ::Outcome<AzFramework::AssetSeedList, AZStd::string> LoadSeedList(const char* fileName, AZStd::string& seedFilepath) override
        {
            if (m_validSeedPath == fileName)
            {
                seedFilepath = fileName;
                return AZ::Success(m_validSeedList);
            }
            return AZ::Failure(AZStd::string("Invalid List"));
        }

        AzFramework::AssetSeedList m_validSeedList;
        AZStd::string m_validSeedPath;
        AZ::Data::AssetId m_assetIds[NumTestAssets];
    };

    struct AssetValidationTest
        : UnitTest::LeakDetectionFixture
        , UnitTest::SetRestoreFileIOBaseRAII
        , AzFramework::ApplicationRequests::Bus::Handler
    {
        AssetValidationTest()
            : UnitTest::SetRestoreFileIOBaseRAII(m_fileIO)
        {
            if (!AZ::SettingsRegistry::Get())
            {
                AZ::SettingsRegistry::Register(&m_registry);

                auto projectPathKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)
                    + "/project_path";
                m_registry.Set(projectPathKey, (AZ::IO::FixedMaxPath(m_tempDir.GetDirectory()) / "AutomatedTesting").Native());
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(m_registry);

                // Set the engine root scan up path to the temporary directory  
                constexpr AZStd::string_view InternalScanUpEngineRootKey{ "/O3DE/Runtime/Internal/engine_root_scan_up_path" };
                m_registry.Set(InternalScanUpEngineRootKey, m_tempDir.GetDirectory());
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(m_registry);
            }
        }

        void NormalizePath(AZStd::string&) override
        {
            AZ_Assert(false, "Not implemented");
        }

        void NormalizePathKeepCase(AZStd::string&) override
        {
            AZ_Assert(false, "Not implemented");
        }

        void CalculateBranchTokenForEngineRoot([[maybe_unused]] AZStd::string& token) const override
        {
            AZ_Assert(false, "Not implemented");
        }

        void SetUp() override
        {
            AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
        }

        bool CreateDummyFile(const AZ::IO::Path& path, AZStd::string_view contents) const;

        AZ::IO::LocalFileIO m_fileIO;
        AZ::Test::ScopedAutoTempDirectory m_tempDir;
        AZ::SettingsRegistryImpl m_registry;
    };
}

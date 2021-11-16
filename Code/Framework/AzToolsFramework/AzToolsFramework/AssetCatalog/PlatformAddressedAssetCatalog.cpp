/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>


namespace AzToolsFramework
{

    PlatformAddressedAssetCatalog::PlatformAddressedAssetCatalog( AzFramework::PlatformId platformId, bool directConnections) :
        m_platformId(platformId),
        AzFramework::AssetCatalog(directConnections)
    {
        InitCatalog();
    }

    PlatformAddressedAssetCatalog::~PlatformAddressedAssetCatalog()
    {
        AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Handler::BusDisconnect(static_cast<int>(m_platformId));
    }

    void PlatformAddressedAssetCatalog::InitCatalog()
    {
        AZStd::string catalogRegistry = GetCatalogRegistryPath();
        bool success = AzFramework::AssetCatalog::LoadCatalog(catalogRegistry.c_str());

        if (!success)
        {
            AZ_Error("PlatformAddressedAssetCatalog", false, "Failed to load catalog.");
            return;
        }

        AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Handler::BusConnect(static_cast<int>(m_platformId));
    }

    const char* GetPlatformName(AzFramework::PlatformId platformNum)
    {
        return AzFramework::PlatformHelper::GetPlatformName(platformNum);
    }


    AZStd::string PlatformAddressedAssetCatalog::GetAssetRootForPlatform(AzFramework::PlatformId platformId)
    {
        AZ::IO::Path projectCachePath;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr
            || !settingsRegistry->Get(projectCachePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            AZ_Warning("PlatformAddressedAssetCatalog", false, "Failed to retrieve valid project cache root from Settings Registry at key %s",
                AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
            return {};
        }

        // Retrieve the ProjectPath/Cache/ path
        AZ::IO::Path platformCachePath = projectCachePath / GetPlatformName(platformId);
        return platformCachePath.Native();
    }

    //! Returns an absolute path to the AssetCatalog for a given platform
    AZStd::string PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId platformId)
    {
        AZ::IO::Path platformCachePath = GetAssetRootForPlatform(platformId);
        return (platformCachePath / "assetcatalog.xml").Native();
    }

    AZStd::string PlatformAddressedAssetCatalog::GetCatalogRegistryPath() const
    {
        return GetCatalogRegistryPathForPlatform(m_platformId);
    }

    bool PlatformAddressedAssetCatalog::CatalogExists(AzFramework::PlatformId platformId)
    {
        return AZ::IO::FileIOBase::GetInstance()->Exists(GetCatalogRegistryPathForPlatform(platformId).c_str());
    }

    void PlatformAddressedAssetCatalog::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        return AssetCatalog::EnableCatalogForAsset(assetType);
    }

    void PlatformAddressedAssetCatalog::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        return AssetCatalog::GetHandledAssetTypes(assetTypes);
    }

    void PlatformAddressedAssetCatalog::DisableCatalog()
    {
        AssetCatalog::DisableCatalog();
    }

    void PlatformAddressedAssetCatalog::StartMonitoringAssets()
    {
        AssetCatalog::StartMonitoringAssets();
    }

    void PlatformAddressedAssetCatalog::StopMonitoringAssets()
    {
        AssetCatalog::StopMonitoringAssets();
    }

    bool PlatformAddressedAssetCatalog::LoadCatalog(const char* catalogRegistryFile)
    {
        return AssetCatalog::LoadCatalog(catalogRegistryFile);
    }

    void PlatformAddressedAssetCatalog::ClearCatalog()
    {
        AssetCatalog::ClearCatalog();
    }

    bool PlatformAddressedAssetCatalog::SaveCatalog(const char* catalogRegistryFile)
    {
        return AssetCatalog::SaveCatalog(catalogRegistryFile);
    }

    bool PlatformAddressedAssetCatalog::AddDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog)
    {
        return AssetCatalog::AddDeltaCatalog(deltaCatalog);
    }

    bool PlatformAddressedAssetCatalog::InsertDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, size_t slotNum)
    {
        return AssetCatalog::InsertDeltaCatalog(deltaCatalog, slotNum);
    }

    bool PlatformAddressedAssetCatalog::InsertDeltaCatalogBefore(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, AZStd::shared_ptr<AzFramework::AssetRegistry> afterDeltaCatalog)
    {
        return AssetCatalog::InsertDeltaCatalogBefore(deltaCatalog, afterDeltaCatalog);
    }

    bool PlatformAddressedAssetCatalog::RemoveDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog)
    {
        return AssetCatalog::RemoveDeltaCatalog(deltaCatalog);
    }

    bool PlatformAddressedAssetCatalog::CreateBundleManifest(const AZStd::string& deltaCatalogPath, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& fileDirectory, int bundleVersion, const AZStd::vector<AZ::IO::Path>& levelDirs)
    {
        return AssetCatalog::CreateBundleManifest(deltaCatalogPath, dependentBundleNames, fileDirectory, bundleVersion, levelDirs);
    }

    bool PlatformAddressedAssetCatalog::CreateDeltaCatalog(const AZStd::vector<AZStd::string>& files, const AZStd::string& filePath)
    {
        return AssetCatalog::CreateDeltaCatalog(files, filePath);
    }

    void PlatformAddressedAssetCatalog::AddExtension(const char* extension)
    {
        AssetCatalog::AddExtension(extension);
    }

    void PlatformAddressedAssetCatalog::RegisterAsset(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& info)
    {
        AssetCatalog::RegisterAsset(id, info);
    }

    void PlatformAddressedAssetCatalog::UnregisterAsset(const AZ::Data::AssetId& id)
    {
        AssetCatalog::UnregisterAsset(id);
    }

    AZStd::string PlatformAddressedAssetCatalog::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        return AssetCatalog::GetAssetPathById(id);
    }

    AZ::Data::AssetInfo PlatformAddressedAssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        return AssetCatalog::GetAssetInfoById(id);
    }

    AZ::Data::AssetId PlatformAddressedAssetCatalog::GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound)
    {
        return AssetCatalog::GetAssetIdByPath(path, typeToRegister, autoRegisterIfNotFound);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> PlatformAddressedAssetCatalog::GetDirectProductDependencies(const AZ::Data::AssetId& asset)
    {
        return AssetCatalog::GetDirectProductDependencies(asset);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> PlatformAddressedAssetCatalog::GetAllProductDependencies(const AZ::Data::AssetId& asset)
    {
        return AssetCatalog::GetAllProductDependencies(asset);
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> PlatformAddressedAssetCatalog::GetAllProductDependenciesFilter(const AZ::Data::AssetId& id, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        return AzFramework::AssetCatalog::GetAllProductDependenciesFilter(id, exclusionList, wildcardPatternExclusionList);
    }


    bool PlatformAddressedAssetCatalog::DoesAssetIdMatchWildcardPattern(const AZ::Data::AssetId& assetId, const AZStd::string& wildcardPattern)
    {
        return AzFramework::AssetCatalog::DoesAssetIdMatchWildcardPattern(assetId, wildcardPattern);
    }

    void PlatformAddressedAssetCatalog::EnumerateAssets(BeginAssetEnumerationCB beginCB, AssetEnumerationCB enumerateCB, EndAssetEnumerationCB endCB)
    {
        AssetCatalog::EnumerateAssets(beginCB, enumerateCB, endCB);
    }
} // namespace AzFramework

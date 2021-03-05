/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>


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
        const char* assetAlias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
        if (assetAlias == nullptr)
        {
            AZ_Error("PlatformAddressedAssetCatalog", false, "Failed to retrieve assetRoot");
            return {};
        }
        AZ::IO::PathView assetRoot{ assetAlias };
        // Folder structure is Cache/Projectfolder/platform/projectfolder
        // It should have at least one path separator
        if (assetRoot.Native().find_first_of(AZ_CORRECT_AND_WRONG_DATABASE_SEPARATOR) == AZStd::string::npos)
        {
            AZ_Warning("PlatformAddressedAssetCatalog", false, "Failed to retrieve valid asset root - got %.*s",
                aznumeric_cast<int>(assetRoot.Native().size()), assetRoot.Native().data());
            return {};
        }

        AZ::IO::PathView projectFolderLower = assetRoot.Filename();
        assetRoot = assetRoot.ParentPath();
        // Retrieve the Cache/Projectfolder path
        AZ::IO::Path platformAssetRoot = assetRoot.ParentPath();
        platformAssetRoot /= GetPlatformName(platformId);
        platformAssetRoot /= projectFolderLower;
        return platformAssetRoot.Native();
    }

    //! Returns an absolute path to the AssetCatalog for a given platform
    AZStd::string PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId platformId)
    {
        AZStd::string assetRoot = GetAssetRootForPlatform(platformId);
        AzFramework::StringFunc::Path::Join(assetRoot.c_str(), "assetcatalog.xml", assetRoot);

        return assetRoot;
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

    bool PlatformAddressedAssetCatalog::CreateBundleManifest(const AZStd::string& deltaCatalogPath, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& fileDirectory, int bundleVersion, const AZStd::vector<AZStd::string>& levelDirs)
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    //! Handle multiple AssetCatalogs with requests addressing each by platform
    class PlatformAddressedAssetCatalogManager : private AzFramework::AssetSystem::NetworkAssetUpdateInterface
    {
    public:
        AZ_TYPE_INFO(PlatformAddressedAssetCatalogManager, "{985263D2-1C04-4811-9EC6-6A069641512A}");
        AZ_CLASS_ALLOCATOR(PlatformAddressedAssetCatalogManager, AZ::SystemAllocator);

        PlatformAddressedAssetCatalogManager();
        PlatformAddressedAssetCatalogManager(AzFramework::PlatformId platformId);
        ~PlatformAddressedAssetCatalogManager();

        using AssetCatalogList = AZStd::vector<AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalog>>;

        void LoadCatalogs();
        void LoadSingleCatalog(AzFramework::PlatformId platformId);
        void TakeSingleCatalog(AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalog>&& newCatalog);
        //////////////////////////////////////////////////////////////////////////
        // NetworkAssetUpdateInterface
        void AssetChanged(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& messages, bool isCatalogInitialize = false) override;
        void AssetRemoved(const AZStd::vector<AzFramework::AssetSystem::AssetNotificationMessage>& messages) override;
        AZStd::string GetSupportedPlatforms() override;
        //////////////////////////////////////////////////////////////////////////

        //! Uses the existence of each platform's AssetCatalog.xml to determine what will be enabled
        //! in order to make this usable without loading any catalogs
        static AZStd::vector< AzFramework::PlatformId> GetEnabledPlatforms();
        static AZStd::string GetEnabledPlatformsString();

        const AssetCatalogList& GetCatalogList() { return m_assetCatalogList; }
    private:
        AssetCatalogList m_assetCatalogList;
    };

} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h>

#include <QAbstractItemModel>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserFavoriteItem;
        class EntryAssetBrowserFavoriteItem;
        class AssetBrowserFavoritesView;

        class AssetBrowserFavoritesManager
            : private AzFramework::AssetCatalogEventBus::Handler
            , private AssetBrowserFavoriteRequestBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserFavoritesManager, AZ::SystemAllocator);

            explicit AssetBrowserFavoritesManager();
            ~AssetBrowserFavoritesManager();

            static void CreateInstance();
            static void DestroyInstance();

            void ClearFavorites();
            void LoadFavorites();

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserFavoriteRequestBus overrides
            //////////////////////////////////////////////////////////////////////////
            void AddFavoriteAsset(const AssetBrowserEntry* favorite) override;
            void AddFavoriteSearchButtonPressed(SearchWidget* searchWidget) override;
            void AddFavoriteEntriesButtonPressed(QWidget* sourceWindow) override;
            void RemoveEntryFromFavorites(const AssetBrowserEntry* favorite) override;
            void RemoveFromFavorites(const AssetBrowserFavoriteItem* favorite) override;
            bool GetIsFavoriteAsset(const AssetBrowserEntry* entry) override;
            void ViewEntryInAssetBrowser(AssetBrowserFavoritesView* targetWindow, const AssetBrowserEntry* favorite) override;
            AZStd::vector<AssetBrowserFavoriteItem*> GetFavorites() override;
            void SaveFavorites() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus::Handler overrides
            //////////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetCatalogBus::Handler overrides
            //////////////////////////////////////////////////////////////////////////
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

            static const char* s_environmentVariableName;
            static AZ::EnvironmentVariable<AssetBrowserFavoritesManager*> g_globalInstance;
        private:
            AZStd::vector<AssetBrowserFavoriteItem*> m_favorites;

            bool m_loading = false;

            AZStd::unordered_map<const AssetBrowserEntry*, AssetBrowserFavoriteItem*> m_favoriteEntriesCache;

            QString GetProjectName();
            void AddFavoriteItem(AssetBrowserFavoriteItem* item);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

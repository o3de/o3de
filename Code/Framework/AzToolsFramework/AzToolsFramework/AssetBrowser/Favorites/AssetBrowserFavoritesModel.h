/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#include <AzCore/Memory/SystemAllocator.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h>

#include <QAbstractItemModel>

#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserFavoriteItem;
        class EntryAssetBrowserFavoriteItem;

        class AssetBrowserFavoritesModel
            : public QAbstractItemModel
            , private AzFramework::AssetCatalogEventBus::Handler
            , private AssetBrowserFavoriteRequestBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserFavoritesModel, AZ::SystemAllocator);

            explicit AssetBrowserFavoritesModel(QObject* parent = nullptr);
            ~AssetBrowserFavoritesModel();

            void SetSearchWidget(SearchWidget* searchWidget);

            void ClearFavorites();
            void LoadFavorites();
            void SaveFavorites();

            void Select(const QModelIndex& favorite);

            //////////////////////////////////////////////////////////////////////////
            // QAbstractItemModel overrides
            //////////////////////////////////////////////////////////////////////////
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& parent = QModelIndex()) const override;
            Qt::ItemFlags flags(const QModelIndex& index) const;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QModelIndex parent(const QModelIndex& child) const override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserModelRequestBus overrides
            //////////////////////////////////////////////////////////////////////////
            void AddFavoriteAsset(const AssetBrowserEntry* favorite) override;
            void AddFavoriteSearchFromWidget(SearchWidget* searchWidget) override;
            void RemoveEntryFromFavorites(const AssetBrowserEntry* favorite) override;
            void RemoveFromFavorites(const AssetBrowserFavoriteItem* favorite) override;
            bool GetIsFavoriteAsset(const AssetBrowserEntry* entry) override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus::Handler overrides
            //////////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetCatalogBus::Handler overrides
            //////////////////////////////////////////////////////////////////////////
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        private:
            AZStd::vector<AssetBrowserFavoriteItem*> m_favorites;

            FolderAssetBrowserEntry* m_favoritesFolderEntry;
            EntryAssetBrowserFavoriteItem* m_favoriteFolderContainer;

            SearchWidget* m_searchWidget = nullptr;

            bool m_loading = false;

            AZStd::unordered_map<const AssetBrowserEntry*, AssetBrowserFavoriteItem*> m_favoriteEntriesCache;

            QString GetProjectName();
            void AddFavoriteItem(AssetBrowserFavoriteItem* item);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

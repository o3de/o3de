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
        class AssetBrowserFavoritesView;
        class EntryAssetBrowserFavoriteItem;
        class SearchAssetBrowserFavoriteItem;

        class AssetBrowserFavoritesModel
            : public QAbstractItemModel
            , private AssetBrowserFavoritesNotificationBus::Handler
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserFavoritesModel, AZ::SystemAllocator);

            explicit AssetBrowserFavoritesModel(QObject* parent = nullptr);
            ~AssetBrowserFavoritesModel();

            void LoadFavorites();
            
            void SetSearchWidget(SearchWidget* searchWidget);

            void Select(AssetBrowserFavoriteItem* favoriteItem);
            void Select(const QModelIndex& favorite);

            void UpdateSearchItem(SearchAssetBrowserFavoriteItem* searchItem);

            void SetParentView(AssetBrowserFavoritesView* parentView);

            void EnableSearchItems();
            void DisableSearchItems();
            bool GetIsSearchDisabled();

            QModelIndex GetTopLevelIndex();

            //////////////////////////////////////////////////////////////////////////
            // QAbstractItemModel overrides
            //////////////////////////////////////////////////////////////////////////
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& parent = QModelIndex()) const override;
            Qt::ItemFlags flags(const QModelIndex& index) const;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QModelIndex parent(const QModelIndex& child) const override;
            Qt::DropActions supportedDropActions() const override;
            bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
            QMimeData* mimeData(const QModelIndexList& indexes) const override;
            QStringList mimeTypes() const override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserFavoritesNotificationBus overrides
            //////////////////////////////////////////////////////////////////////////
            void FavoritesChanged() override;

        private:
            AZStd::vector<AssetBrowserFavoriteItem*> m_favorites;

            FolderAssetBrowserEntry* m_favoritesFolderEntry;
            EntryAssetBrowserFavoriteItem* m_favoriteFolderContainer;

            SearchWidget* m_searchWidget = nullptr;

            AssetBrowserFavoritesView* m_parentView = nullptr;

            bool m_loading = false;
            bool m_modifying = false;
            bool m_searchDisabled = false;

            QString GetProjectName();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

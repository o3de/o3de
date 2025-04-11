/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetBrowserFavoritesModel.h"

#include <AzCore/Utils/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesView.h>
#include <AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

#include <QModelIndex>
#include <QSettings>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* const IconPath = ":/Gallery/Favorite_Folder.svg";
        AssetBrowserFavoritesModel::AssetBrowserFavoritesModel(QObject* parent)
            : QAbstractItemModel(parent)
            , m_favoritesFolderEntry(aznew FolderAssetBrowserEntry)
            , m_favoriteFolderContainer(aznew EntryAssetBrowserFavoriteItem)
        {
            AssetBrowserFavoritesNotificationBus::Handler::BusConnect();

            m_favoritesFolderEntry->SetDisplayName(QObject::tr("Favorites"));
            m_favoritesFolderEntry->SetIconPath(IconPath);

            m_favoriteFolderContainer->SetEntry(m_favoritesFolderEntry);
        }

        AssetBrowserFavoritesModel ::~AssetBrowserFavoritesModel()
        {
            AssetBrowserFavoritesNotificationBus::Handler::BusDisconnect();
        }
        
        QModelIndex AssetBrowserFavoritesModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            if (!parent.isValid())
            {
                return createIndex(0, 0);
            }

            AssetBrowserFavoriteItem* childEntry = m_favorites.at(row);
            // cast the const away purely so createIndex works. Entry will not be modified.
            return createIndex(row, column, childEntry);
        }

        int AssetBrowserFavoritesModel::rowCount(const QModelIndex& parent) const
        {
            if (!parent.isValid())
            {
                return 1;
            }

            if (parent.internalPointer())
            {
                return 0;
            }

            return aznumeric_cast<int>(m_favorites.size());
        }

        int AssetBrowserFavoritesModel::columnCount([[maybe_unused]]const QModelIndex& parent) const
        {
            return 1;
        }

        Qt::ItemFlags AssetBrowserFavoritesModel::flags(const QModelIndex& index) const
        {
            Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

            if (index.isValid())
            {
                if (index.parent().isValid())
                {
                    AssetBrowserFavoriteItem* item = m_favorites.at(index.row());
                    if (item->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                    {
                        defaultFlags |= Qt::ItemIsEditable;
                    }
                    else
                    {
                        EntryAssetBrowserFavoriteItem* favoriteItem = static_cast<EntryAssetBrowserFavoriteItem*>(item);
                        const AssetBrowserEntry* entry = favoriteItem->GetEntry();

                        if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Product ||
                            entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source)
                        {
                            return Qt::ItemIsDragEnabled | defaultFlags;
                        }
                    }
                }
            }
            return defaultFlags;
        }

        QVariant AssetBrowserFavoritesModel::data(const QModelIndex& index, int role) const
        {
            if (!index.isValid())
            {
                return QVariant();
            }
            
            if (role == Qt::DisplayRole)
            {
                if (index.parent().isValid())
                {
                    AssetBrowserFavoriteItem* item = m_favorites.at(index.row());
                    if (item->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                    {
                        SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(item);
                        return searchItem->GetName();
                    }
                    else if (item->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
                    {
                        EntryAssetBrowserFavoriteItem* entryItem = static_cast<EntryAssetBrowserFavoriteItem*>(item);
                        return entryItem->GetEntry()->GetDisplayName();
                    }
                }
                return "";
            }

            if (role == Qt::EditRole)
            {
                if (index.parent().isValid())
                {
                    AssetBrowserFavoriteItem* item = m_favorites.at(index.row());
                    if (item->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                    {
                        SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(item);
                        return searchItem->GetName();
                    }
                }
            }

            if (role == AssetBrowserModel::Roles::EntryRole)
            {
                if (index.parent().isValid())
                {
                    const AssetBrowserFavoriteItem* item = static_cast<AssetBrowserFavoriteItem*>(index.internalPointer());
                    return QVariant::fromValue(item);
                }
                else
                {
                    const AssetBrowserFavoriteItem* item = static_cast<AssetBrowserFavoriteItem*>(m_favoriteFolderContainer);
                    return QVariant::fromValue(item);
                }

            }

            return QVariant();
        }

        QModelIndex AssetBrowserFavoritesModel::parent(const QModelIndex& child) const
        {
            if (child.internalPointer())
            {
                return createIndex(0, 0);
            }
            return QModelIndex();
        }

        QStringList AssetBrowserFavoritesModel::mimeTypes() const
        {
            QStringList list = QAbstractItemModel::mimeTypes();
            list.append(AssetBrowserEntry::GetMimeType());
            return list;
        }

        bool AssetBrowserFavoritesModel::dropMimeData(
            const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
        {
            if (action == Qt::IgnoreAction)
                return true;

            AZStd::vector<const AssetBrowserEntry*> entries;

            if (Utils::FromMimeData(data, entries))
            {
                for (auto entry : entries)
                {
                    if (!entry)
                    {
                        continue;
                    }
                    if (entry->RTTI_IsTypeOf(SourceAssetBrowserEntry::RTTI_Type()) ||
                        entry->RTTI_IsTypeOf(ProductAssetBrowserEntry::RTTI_Type()) ||
                        entry->RTTI_IsTypeOf(FolderAssetBrowserEntry::RTTI_Type()))
                    {
                        AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::AddFavoriteAsset, entry);
                    }
                }
                return true;
            }

            return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
        }

        Qt::DropActions AssetBrowserFavoritesModel::supportedDropActions() const
        {
            return Qt::CopyAction;
        }

        QMimeData* AssetBrowserFavoritesModel::mimeData(const QModelIndexList& indexes) const
        {
            QMimeData* mimeData = new QMimeData;

            AZStd::vector<const AssetBrowserEntry*> collected;
            collected.reserve(indexes.size());

            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    const AssetBrowserFavoriteItem* item = static_cast<const AssetBrowserFavoriteItem*>(index.internalPointer());
                    if (item->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
                    {
                        const EntryAssetBrowserFavoriteItem* favoriteItem = static_cast<const EntryAssetBrowserFavoriteItem*>(item);
                        const AssetBrowserEntry* entry = favoriteItem->GetEntry();
                        if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Product ||
                            entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source)
                        {
                            collected.push_back(entry);
                        }
                    }
                }
            }

            Utils::ToMimeData(mimeData, collected);

            return mimeData;
        }

        void AssetBrowserFavoritesModel::SetSearchWidget(SearchWidget* searchWidget)
        {
            m_searchWidget = searchWidget;

            LoadFavorites();
        }

        void AssetBrowserFavoritesModel::FavoritesChanged()
        {
            if (m_modifying)
            {
                return;
            }

            LoadFavorites();
        }

        void AssetBrowserFavoritesModel::Select(AssetBrowserFavoriteItem* favoriteItem)
        {
            if (!favoriteItem)
            {
                return;
            }

            QWidget* parentWidget = m_searchWidget->parentWidget();

            if (favoriteItem->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
            {
                EntryAssetBrowserFavoriteItem* favoriteEntry = static_cast<EntryAssetBrowserFavoriteItem*>(favoriteItem);

                const AssetBrowserEntry* selectedEntry = favoriteEntry->GetEntry();

                AZStd::string fullPath = static_cast<const SourceAssetBrowserEntry*>(selectedEntry)->GetFullPath();

                if (selectedEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    AssetBrowserInteractionNotificationBus::Broadcast(
                        &AssetBrowserInteractionNotificationBus::Events::SelectFolderAsset, parentWidget, fullPath);
                }
                else
                {
                    AssetBrowserInteractionNotificationBus::Broadcast(
                        &AssetBrowserInteractionNotificationBus::Events::SelectAsset, parentWidget, fullPath);
                }
            }

            if (favoriteItem->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
            {
                SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(favoriteItem);

                m_searchWidget->ClearTextFilter();
                m_searchWidget->ClearTypeFilter();

                searchItem->WriteToSearchWidget(m_searchWidget);
            }
        }

        void AssetBrowserFavoritesModel::Select(const QModelIndex& favorite)
        {
            if (!favorite.internalPointer())
            {
                return;
            }

            AssetBrowserFavoriteItem* favoriteItem = static_cast<AssetBrowserFavoriteItem*>(favorite.internalPointer());
            Select(favoriteItem);
        }

        void AssetBrowserFavoritesModel::UpdateSearchItem(SearchAssetBrowserFavoriteItem* searchItem)
        {
            searchItem->SetupFromSearchWidget(m_searchWidget);

            AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::SaveFavorites);
        }

        void AssetBrowserFavoritesModel::SetParentView(AssetBrowserFavoritesView* parentView)
        {
            m_parentView = parentView;
        }

        void AssetBrowserFavoritesModel::EnableSearchItems()
        {
            m_searchDisabled = false;
        }

        void AssetBrowserFavoritesModel::DisableSearchItems()
        {
            m_searchDisabled = true;
        }

        bool AssetBrowserFavoritesModel::GetIsSearchDisabled()
        {
            return m_searchDisabled;
        }

        QModelIndex AssetBrowserFavoritesModel::GetTopLevelIndex()
        {
            return createIndex(0, 0);
        }

        QString AssetBrowserFavoritesModel::GetProjectName()
        {
            AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
            if (!projectName.empty())
            {
                return QString::fromUtf8(projectName.c_str(), aznumeric_cast<int>(projectName.size()));
            }

            return "unknown";
        }

        void AssetBrowserFavoritesModel::LoadFavorites()
        {
            m_loading = true;

            beginResetModel();

            AssetBrowserFavoriteRequestBus::BroadcastResult(m_favorites, &AssetBrowserFavoriteRequestBus::Events::GetFavorites);
                
            endResetModel();

            m_loading = false;

            m_parentView->expand(GetTopLevelIndex());
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Favorites/moc_AssetBrowserFavoritesModel.cpp"

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
            AssetBrowserFavoriteRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();

            m_favoritesFolderEntry->SetDisplayName(QObject::tr("Favorites"));
            m_favoritesFolderEntry->SetIconPath(IconPath);

            m_favoriteFolderContainer->SetEntry(m_favoritesFolderEntry);
        }

        AssetBrowserFavoritesModel ::~AssetBrowserFavoritesModel()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
            AssetBrowserFavoriteRequestBus::Handler::BusDisconnect();
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
                    if (item->RTTI_IsTypeOf(ProductAssetBrowserEntry::RTTI_Type()) ||
                        item->RTTI_IsTypeOf(SourceAssetBrowserEntry::RTTI_Type()))
                    {
                        return Qt::ItemIsDragEnabled | defaultFlags;
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
                         entry->RTTI_IsTypeOf(ProductAssetBrowserEntry::RTTI_Type()))
                    {
                        AddFavoriteAsset(entry);
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
                    const AssetBrowserEntry* item = static_cast<const AssetBrowserEntry*>(index.internalPointer());
                    if (item)
                    {
                        collected.push_back(item);
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


        void AssetBrowserFavoritesModel::AddFavoriteItem(AssetBrowserFavoriteItem* item)
        {
            int rowCount = aznumeric_cast<int>(m_favorites.size());

            beginInsertRows(createIndex(0, 0), rowCount, rowCount);

            m_favorites.push_back(item);

            endInsertRows();

            SaveFavorites();
        }

        void AssetBrowserFavoritesModel::AddFavoriteAsset(const AssetBrowserEntry* favorite)
        {
            if (GetIsFavoriteAsset(favorite))
            {
                return;
            }

            EntryAssetBrowserFavoriteItem* item = aznew EntryAssetBrowserFavoriteItem();
            item->SetEntry(favorite);

            AddFavoriteItem(item);

            m_favoriteEntriesCache[favorite] = item;
        }

        void AssetBrowserFavoritesModel::AddFavoriteSearchFromWidget(SearchWidget* searchWidget)
        {
            SearchAssetBrowserFavoriteItem* item = aznew SearchAssetBrowserFavoriteItem(searchWidget);
            item->SetupFromSearchWidget(searchWidget);
            item->SetName(QObject::tr("New Saved Search"));

            AddFavoriteItem(item);
        }

        void AssetBrowserFavoritesModel::RemoveEntryFromFavorites(const AssetBrowserEntry* item)
        {
            const auto favoriteIt = m_favoriteEntriesCache.find(item);
            if (favoriteIt != m_favoriteEntriesCache.end() || !favoriteIt->second)
            {
                auto removeIt = AZStd::find(m_favorites.begin(), m_favorites.end(), favoriteIt->second);

                if (removeIt == m_favorites.end())
                {
                    AZ_Assert(false, "Unknown favorite item");
                    return;
                }

                m_favorites.erase(removeIt);
                m_favoriteEntriesCache.erase(favoriteIt);
            }
        }

        void AssetBrowserFavoritesModel::RemoveFromFavorites(const AssetBrowserFavoriteItem* favorite)
        {
            if (favorite->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
            {
                const EntryAssetBrowserFavoriteItem* entryItem = static_cast<const EntryAssetBrowserFavoriteItem*>(favorite);
                RemoveEntryFromFavorites(entryItem->GetEntry());
                return;
            }

            auto removeIt = AZStd::find(m_favorites.begin(), m_favorites.end(), favorite);

            if (removeIt == m_favorites.end())
            {
                AZ_Assert(false, "Unknown favorite item");
                return;
            }

            m_favorites.erase(removeIt);
        }

        bool AssetBrowserFavoritesModel::GetIsFavoriteAsset(const AssetBrowserEntry* entry)
        {
            const auto it = m_favoriteEntriesCache.find(entry);
            return (it != m_favoriteEntriesCache.end());
        }

        void AssetBrowserFavoritesModel::Select(const QModelIndex& favorite)
        {
            if (!favorite.internalPointer())
            {
                return;
            }

            AssetBrowserFavoriteItem* favoriteItem = static_cast<AssetBrowserFavoriteItem*>(favorite.internalPointer());
            if (favoriteItem->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
            {
                EntryAssetBrowserFavoriteItem* favoriteEntry = static_cast<EntryAssetBrowserFavoriteItem*>(favoriteItem);

                const AssetBrowserEntry* selectedEntry = favoriteEntry->GetEntry();
                if (selectedEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                {
                    AssetBrowserViewRequestBus::Broadcast(
                        &AssetBrowserViewRequestBus::Events::SelectProduct,
                        static_cast<const ProductAssetBrowserEntry*>(selectedEntry)->GetAssetId());
                    
                }
                if (selectedEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                {
                    //TODO - neeeds to go to correct view.
                    AZStd::string fullPath = static_cast<const SourceAssetBrowserEntry*>(selectedEntry)->GetFullPath();

                    AssetBrowserViewRequestBus::Broadcast(
                        &AssetBrowserViewRequestBus::Events::SelectFileAtPath,
                        AZ::IO::PathView(fullPath.data()).LexicallyNormal().String());
                }
                if (selectedEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    AZStd::string fullPath = static_cast<const FolderAssetBrowserEntry*>(selectedEntry)->GetFullPath();

                    AssetBrowserViewRequestBus::Broadcast(
                        &AssetBrowserViewRequestBus::Events::SelectFileAtPath,
                        AZ::IO::PathView(fullPath.data()).LexicallyNormal().String());
                }
                    
            }

            if (favoriteItem->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
            {
                SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(favoriteItem);

                searchItem->WriteToSearchWidget(m_searchWidget);
            }
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

        void AssetBrowserFavoritesModel::ClearFavorites()
        {
            if (m_favorites.empty())
            {
                return;
            }

            beginRemoveRows(createIndex(0, 0), 0, aznumeric_cast<int>(m_favorites.size()));

            m_favoriteEntriesCache.clear();

            m_favorites.erase(m_favorites.begin(), m_favorites.end());

            endRemoveRows();
        }

        void AssetBrowserFavoritesModel::LoadFavorites()
        {
            m_loading = true;

            beginResetModel();

            ClearFavorites();

            QSettings settings;
            settings.beginGroup("AssetBrowserFavorites");

            QString projectName = GetProjectName();
            if (projectName.length() > 0)
            {
                settings.beginGroup(GetProjectName());
            }

            int size = settings.beginReadArray("Items");

            for (int index = 0; index < size; index++)
            {
                settings.setArrayIndex(index);

                QString path = settings.value("entryPath", "").toString();
                if (!path.isEmpty())
                {
                    AZStd::string filePath = AZ::IO::PathView(path.toUtf8().data()).LexicallyNormal().String();

                    const auto itFile = EntryCache::GetInstance()->m_absolutePathToFileId.find(filePath);
                    if (itFile == EntryCache::GetInstance()->m_absolutePathToFileId.end())
                    {
                        continue;
                    }

                    const auto itABEntry = EntryCache::GetInstance()->m_fileIdMap.find(itFile->second);
                    if (itABEntry == EntryCache::GetInstance()->m_fileIdMap.end())
                    {
                        continue;
                    }

                    AddFavoriteAsset(itABEntry->second);
                }

                bool isSearch = settings.value("favoriteSearch", false).toBool();
                if (isSearch)
                {
                    SearchAssetBrowserFavoriteItem* search = aznew SearchAssetBrowserFavoriteItem(m_searchWidget);
                    search->LoadSettings(settings);
                    AddFavoriteItem(search);
                }
            }
                
            endResetModel();

            m_loading = false;
        }

        void AssetBrowserFavoritesModel::SaveFavorites()
        {
            if (m_loading)
            {
                return;
            }

            QSettings settings;
            settings.beginGroup("AssetBrowserFavorites");

            QString projectName = GetProjectName();
            if (projectName.length() > 0)
            {
                // Clear the group
                settings.beginGroup(projectName);
                settings.remove("");
                settings.endGroup();

                settings.beginGroup(projectName);
            }

            settings.remove("");
            settings.beginWriteArray("Items");

            for (size_t index = 0; index < m_favorites.size(); index++)
            {
                AssetBrowserFavoriteItem* entry = m_favorites.at(index);

                settings.setArrayIndex(aznumeric_cast<int>(index));

                if (entry->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
                {
                    EntryAssetBrowserFavoriteItem* entryItem = static_cast<EntryAssetBrowserFavoriteItem*>(entry);
                    settings.setValue("entryPath", entryItem->GetEntry()->GetFullPath().data());
                }
                else if (entry->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                {
                    SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(entry);
                    settings.setValue("favoriteSearch", true);
                    searchItem->SaveSettings(settings);
                }
            }

            settings.endArray();
        }

        void AssetBrowserFavoritesModel::OnAssetBrowserComponentReady()
        {
            LoadFavorites();
        }

        void AssetBrowserFavoritesModel::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
        {
            // Find the source entry for this asset.
            const auto sourceIt = EntryCache::GetInstance()->m_sourceUuidMap.find(assetId.m_guid);
            if (sourceIt == EntryCache::GetInstance()->m_sourceUuidMap.end())
            {
                return;
            }

            AssetBrowserEntry* entry = sourceIt->second;

            RemoveEntryFromFavorites(entry);
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Favorites/moc_AssetBrowserFavoritesModel.cpp"

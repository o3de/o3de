/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetBrowserFavoritesManager.h"

#include <AzCore/Utils/Utils.h>
#include <AzCore/Module/Environment.h>
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
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesModel.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesView.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>

#include <QModelIndex>
#include <QSettings>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const char* AssetBrowserFavoritesManager::s_environmentVariableName = "AssetBrowserFavoritesManager";
        AZ::EnvironmentVariable<AssetBrowserFavoritesManager*> AssetBrowserFavoritesManager::g_globalInstance;

        AssetBrowserFavoritesManager::AssetBrowserFavoritesManager()
        {
            AssetBrowserFavoriteRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        AssetBrowserFavoritesManager ::~AssetBrowserFavoritesManager()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
            AssetBrowserFavoriteRequestBus::Handler::BusDisconnect();
        }

        void AssetBrowserFavoritesManager::AddFavoriteItem(AssetBrowserFavoriteItem* item)
        {
            m_favorites.push_back(item);

            SaveFavorites();
        }

        void AssetBrowserFavoritesManager::AddFavoriteAsset(const AssetBrowserEntry* favorite)
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

        void AssetBrowserFavoritesManager::AddFavoriteSearchButtonPressed(SearchWidget* searchWidget)
        {
            SearchAssetBrowserFavoriteItem* item = aznew SearchAssetBrowserFavoriteItem();
            item->SetupFromSearchWidget(searchWidget);

            if (!item->IsFilterActive())
            {
                delete item;
                return;
            }

            AddFavoriteItem(item);
        }

        void AssetBrowserFavoritesManager::AddFavoriteEntriesButtonPressed(QWidget* sourceWindow)
        {
            AZStd::vector<const AssetBrowserEntry*> entries;

            AssetBrowserTreeView* treeView = qobject_cast<AssetBrowserTreeView*>(sourceWindow);
            if (treeView)
            {
                entries = treeView->GetSelectedAssets();
            }

            AssetBrowserTableView* tableView = qobject_cast<AssetBrowserTableView*>(sourceWindow);
            if (tableView)
            {
                entries = tableView->GetSelectedAssets();
            }

            AssetBrowserThumbnailView* thumbnailView = qobject_cast<AssetBrowserThumbnailView*>(sourceWindow);
            if (thumbnailView)
            {
                entries = thumbnailView->GetSelectedAssets();
            }

            if (entries.empty())
            {
                return;
            }

            m_loading = true;

            for (auto entry : entries)
            {
                AddFavoriteAsset(entry);
            }

            m_loading = false;

            SaveFavorites();
        }

        void AssetBrowserFavoritesManager::RemoveEntryFromFavorites(const AssetBrowserEntry* item)
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

                SaveFavorites();
            }
        }

        void AssetBrowserFavoritesManager::ViewEntryInAssetBrowser(AssetBrowserFavoritesView* targetWindow, const AssetBrowserEntry* favorite)
        {
            const auto favoriteIt = m_favoriteEntriesCache.find(favorite);
            if (favoriteIt != m_favoriteEntriesCache.end() || !favoriteIt->second)
            {
                AssetBrowserFavoritesModel* model = targetWindow->GetModel();
                if (model)
                {
                    model->Select(favoriteIt->second);
                }
            }
        }

        void AssetBrowserFavoritesManager::RemoveFromFavorites(const AssetBrowserFavoriteItem* favorite)
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

            SaveFavorites();
        }

        bool AssetBrowserFavoritesManager::GetIsFavoriteAsset(const AssetBrowserEntry* entry)
        {
            const auto it = m_favoriteEntriesCache.find(entry);
            return (it != m_favoriteEntriesCache.end());
        }

        QString AssetBrowserFavoritesManager::GetProjectName()
        {
            AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
            if (!projectName.empty())
            {
                return QString::fromUtf8(projectName.c_str(), aznumeric_cast<int>(projectName.size()));
            }

            return "unknown";
        }

        void AssetBrowserFavoritesManager::ClearFavorites()
        {
            if (m_favorites.empty())
            {
                return;
            }

            m_favoriteEntriesCache.clear();

            m_favorites.erase(m_favorites.begin(), m_favorites.end());
        }

        void AssetBrowserFavoritesManager::LoadFavorites()
        {
            m_loading = true;

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
                    SearchAssetBrowserFavoriteItem* search = aznew SearchAssetBrowserFavoriteItem();
                    search->LoadSettings(settings);
                    AddFavoriteItem(search);
                }
            }

            m_loading = false;

            AssetBrowserFavoritesNotificationBus::Broadcast(&AssetBrowserFavoritesNotificationBus::Events::FavoritesChanged);
        }

        void AssetBrowserFavoritesManager::SaveFavorites()
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

            AssetBrowserFavoritesNotificationBus::Broadcast(&AssetBrowserFavoritesNotificationBus::Events::FavoritesChanged);
        }

        void AssetBrowserFavoritesManager::OnAssetBrowserComponentReady()
        {
            LoadFavorites();
        }

        void AssetBrowserFavoritesManager::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
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

        AZStd::vector<AssetBrowserFavoriteItem*> AssetBrowserFavoritesManager::GetFavorites()
        {
            return m_favorites;
        }

        void AssetBrowserFavoritesManager::CreateInstance()
        {
            if (!g_globalInstance)
            {
                g_globalInstance = AZ::Environment::CreateVariable<AssetBrowserFavoritesManager*>(s_environmentVariableName);
                (*g_globalInstance) = nullptr;
            }

            AZ_Assert(!(*g_globalInstance), "You may not Create instance twice.");

            AssetBrowserFavoritesManager* newInstance = aznew AssetBrowserFavoritesManager();
            (*g_globalInstance) = newInstance;
        }

        void AssetBrowserFavoritesManager::DestroyInstance()
        {
            AZ_Assert(g_globalInstance, "Invalid call to DestroyInstance - no instance exists.");
            AZ_Assert(*g_globalInstance, "You can only call DestroyInstance if you have called CreateInstance.");
            if (g_globalInstance)
            {
                delete *g_globalInstance;
            }
            (*g_globalInstance) = nullptr;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

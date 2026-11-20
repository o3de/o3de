/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetBrowserFavoritesManager.h"

#include <AzCore/Settings/SettingsRegistry.h>
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
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesSettings.h>
#include <AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesModel.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesView.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>

#include <QModelIndex>

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

            // If this asset was previously unresolved, remove it from the unresolved list
            AZStd::string normalizedPath = AZ::IO::PathView(favorite->GetFullPath()).LexicallyNormal().String();
            m_unresolvedFavoritePaths.erase(
                AZStd::remove(m_unresolvedFavoritePaths.begin(), m_unresolvedFavoritePaths.end(), normalizedPath),
                m_unresolvedFavoritePaths.end());
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

        // ============================================================
        //  Settings Registry Path Helper
        // ============================================================
        //
        //  Favorites are stored per-project in:
        //      <ProjectPath>/user/Registry/editorpreferences.setreg
        //  under the registry path:
        //      /O3DE/Preferences/AssetBrowser/Favorites/<ProjectName>
        //
        //  Replaces the previous QSettings backing, which was platform-
        //  specific (Windows registry on Windows) and global to the
        //  machine rather than scoped to the project.
        // ============================================================
        static AZStd::string BuildFavoritesRegistryPath(const QString& projectName)
        {
            AZStd::string path = AssetBrowserFavoritesRegistryPrefix;
            if (!projectName.isEmpty())
            {
                path += "/";
                path += projectName.toUtf8().constData();
            }
            return path;
        }

        // ============================================================
        //  Load Favorites from Settings Registry
        // ============================================================
        void AssetBrowserFavoritesManager::LoadFavorites()
        {
            m_loading = true;

            ClearFavorites();
            m_unresolvedFavoritePaths.clear();

            auto* registry = AZ::SettingsRegistry::Get();
            if (registry == nullptr)
            {
                m_loading = false;
                AssetBrowserFavoritesNotificationBus::Broadcast(&AssetBrowserFavoritesNotificationBus::Events::FavoritesChanged);
                return;
            }

            const AZStd::string registryPath = BuildFavoritesRegistryPath(GetProjectName());

            AssetBrowserFavoritesProjectSettings projectSettings;
            registry->GetObject(projectSettings, registryPath);

            for (const FavoriteRecord& record : projectSettings.m_items)
            {
                if (record.m_isSearch)
                {
                    SearchAssetBrowserFavoriteItem* search = aznew SearchAssetBrowserFavoriteItem();
                    search->LoadFromRecord(record);
                    AddFavoriteItem(search);
                    continue;
                }

                if (record.m_entryPath.empty())
                {
                    continue;
                }

                AZStd::string filePath = AZ::IO::PathView(record.m_entryPath).LexicallyNormal().String();

                const auto itFile = EntryCache::GetInstance()->m_absolutePathToFileId.find(filePath);
                if (itFile == EntryCache::GetInstance()->m_absolutePathToFileId.end())
                {
                    // Cache not populated yet - keep so it is not lost on next save
                    m_unresolvedFavoritePaths.push_back(filePath);
                    continue;
                }

                const auto itABEntry = EntryCache::GetInstance()->m_fileIdMap.find(itFile->second);
                if (itABEntry == EntryCache::GetInstance()->m_fileIdMap.end())
                {
                    m_unresolvedFavoritePaths.push_back(filePath);
                    continue;
                }

                AddFavoriteAsset(itABEntry->second);
            }

            // Carry forward any previously unresolved paths that this session also could not resolve.
            for (const AZStd::string& path : projectSettings.m_unresolvedPaths)
            {
                if (AZStd::find(m_unresolvedFavoritePaths.begin(), m_unresolvedFavoritePaths.end(), path)
                    == m_unresolvedFavoritePaths.end())
                {
                    m_unresolvedFavoritePaths.push_back(path);
                }
            }

            m_loading = false;

            AssetBrowserFavoritesNotificationBus::Broadcast(&AssetBrowserFavoritesNotificationBus::Events::FavoritesChanged);
        }

        // ============================================================
        //  Save Favorites to Settings Registry
        // ============================================================
        void AssetBrowserFavoritesManager::SaveFavorites()
        {
            if (m_loading)
            {
                return;
            }

            auto* registry = AZ::SettingsRegistry::Get();
            if (registry == nullptr)
            {
                AZ_Warning("AssetBrowserFavorites", false,
                    "Settings registry unavailable; favorites will not be persisted.");
                return;
            }

            AssetBrowserFavoritesProjectSettings projectSettings;
            projectSettings.m_items.reserve(m_favorites.size());

            for (AssetBrowserFavoriteItem* entry : m_favorites)
            {
                FavoriteRecord& record = projectSettings.m_items.emplace_back();

                if (entry->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
                {
                    EntryAssetBrowserFavoriteItem* entryItem = static_cast<EntryAssetBrowserFavoriteItem*>(entry);
                    record.m_isSearch = false;
                    record.m_entryPath = entryItem->GetEntry()->GetFullPath();
                }
                else if (entry->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                {
                    SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(entry);
                    searchItem->SaveToRecord(record);
                }
            }

            projectSettings.m_unresolvedPaths = m_unresolvedFavoritePaths;

            const AZStd::string registryPath = BuildFavoritesRegistryPath(GetProjectName());

            // Clear the prior subtree so removed entries don't linger.
            registry->Remove(registryPath);

            if (!registry->SetObject(registryPath, projectSettings))
            {
                AZ_Warning("AssetBrowserFavorites", false,
                    "Failed to write favorites to settings registry at %s", registryPath.c_str());
            }

            // Flush editorpreferences.setreg through the editor settings API so
            // the per-project user folder reflects the change immediately.
            EditorSettingsAPIBus::Broadcast(&EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);

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

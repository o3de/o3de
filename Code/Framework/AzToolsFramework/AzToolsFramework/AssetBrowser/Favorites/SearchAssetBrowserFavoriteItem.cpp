/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h"
#include "AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h"
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesSettings.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SearchAssetBrowserFavoriteItem::SearchAssetBrowserFavoriteItem()
        {
        }

        SearchAssetBrowserFavoriteItem::~SearchAssetBrowserFavoriteItem()
        {
            qDeleteAll(m_typeFilters.begin(), m_typeFilters.end());
            m_typeFilters.clear();
        }

        void SearchAssetBrowserFavoriteItem::SetName(const QString name)
        {
            m_name = name;
        }

        const QString SearchAssetBrowserFavoriteItem::GetName() const
        {
            return m_name;
        }

        AssetBrowserFavoriteItem::FavoriteType SearchAssetBrowserFavoriteItem::GetFavoriteType() const
        {
            return FavoriteType::Search;
        }

        bool SearchAssetBrowserFavoriteItem::IsFilterActive()
        {
            if (!m_searchTerm.isEmpty())
            {
                return true;
            }

            for (auto typeFilter : m_typeFilters)
            {
                if (typeFilter->enabled)
                {
                    return true;
                }
            }

            return false;
        }

        void SearchAssetBrowserFavoriteItem::SetupFromSearchWidget(SearchWidget* searchWidget)
        {
            m_unusableProductsFilterActive = searchWidget->GetIsUnusableProductsFilterActive();
            m_folderFilterActive = searchWidget->GetIsFolderFilterActive();
            m_engineFilterActive = searchWidget->GetIsEngineFilterActive();

            m_searchTerm = searchWidget->GetFilterString();

            int numTypeFilters = searchWidget->GetTypeFilterCount();

            qDeleteAll(m_typeFilters.begin(), m_typeFilters.end());
            m_typeFilters.clear();
            m_typeFilters.reserve(numTypeFilters);

            for (int typeIndex = 0; typeIndex < numTypeFilters; typeIndex++)
            {
                SavedTypeFilter* typeFilter = aznew SavedTypeFilter();
                searchWidget->GetTypeFilterDetails(typeIndex, typeFilter->categoryKey, typeFilter->displayName, typeFilter->enabled);
                m_typeFilters.append(typeFilter);
            }

            m_name = GetDefaultName();
        }

        void SearchAssetBrowserFavoriteItem::WriteToSearchWidget(SearchWidget* searchWidget)
        {
            searchWidget->ToggleUnusableProductsFilter(m_unusableProductsFilterActive);
            searchWidget->ToggleEngineFilter(m_engineFilterActive);

            if (m_folderFilterActive)
            {
                searchWidget->AddFolderFilter();
            }
            else
            {
                searchWidget->RemoveFolderFilter();
            }

            for (auto typeFilter : m_typeFilters)
            {
                searchWidget->SetFilterState(typeFilter->categoryKey, typeFilter->displayName, typeFilter->enabled);
            }

            searchWidget->SetFilterString(m_searchTerm);
        }

        void SearchAssetBrowserFavoriteItem::LoadFromRecord(const FavoriteRecord& record)
        {
            m_name = QString::fromUtf8(record.m_name.c_str());
            m_searchTerm = QString::fromUtf8(record.m_searchTerm.c_str());
            m_unusableProductsFilterActive = record.m_unusableProductsFilterActive;
            m_engineFilterActive = record.m_engineFilterActive;
            m_folderFilterActive = record.m_folderFilterActive;

            qDeleteAll(m_typeFilters.begin(), m_typeFilters.end());
            m_typeFilters.clear();
            m_typeFilters.reserve(static_cast<int>(record.m_typeFilters.size()));

            for (const FavoriteSearchTypeFilterRecord& filterRecord : record.m_typeFilters)
            {
                SavedTypeFilter* typeFilter = aznew SavedTypeFilter();
                typeFilter->categoryKey = QString::fromUtf8(filterRecord.m_categoryKey.c_str());
                typeFilter->displayName = QString::fromUtf8(filterRecord.m_displayName.c_str());
                typeFilter->enabled = filterRecord.m_enabled;
                m_typeFilters.append(typeFilter);
            }
        }

        void SearchAssetBrowserFavoriteItem::SaveToRecord(FavoriteRecord& record) const
        {
            record.m_isSearch = true;
            record.m_name = m_name.toUtf8().constData();
            record.m_searchTerm = m_searchTerm.toUtf8().constData();
            record.m_unusableProductsFilterActive = m_unusableProductsFilterActive;
            record.m_engineFilterActive = m_engineFilterActive;
            record.m_folderFilterActive = m_folderFilterActive;

            record.m_typeFilters.clear();
            record.m_typeFilters.reserve(m_typeFilters.size());

            for (const SavedTypeFilter* typeFilter : m_typeFilters)
            {
                FavoriteSearchTypeFilterRecord& filterRecord = record.m_typeFilters.emplace_back();
                filterRecord.m_categoryKey = typeFilter->categoryKey.toUtf8().constData();
                filterRecord.m_displayName = typeFilter->displayName.toUtf8().constData();
                filterRecord.m_enabled = typeFilter->enabled;
            }
        }

        QString SearchAssetBrowserFavoriteItem::GetDefaultName()
        {
            QString name = "";

            if (!m_searchTerm.isEmpty())
            {
                name += "S:" + m_searchTerm;
            }

            bool firstEntry = true;
            for (auto typeFilter : m_typeFilters)
            {
                if (typeFilter->enabled)
                {
                    if (firstEntry)
                    {
                        if (!name.isEmpty())
                        {
                            name += "&";
                        }
                        firstEntry = false;
                    }
                    else
                    {
                        name += "|";
                    }
                    name += "F:" + typeFilter->displayName;
                }
            }

            return name;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

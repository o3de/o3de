/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h"
#include "AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h"
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SearchAssetBrowserFavoriteItem::SearchAssetBrowserFavoriteItem(SearchWidget* searchWidget)
        {
            int numTypeFilters = searchWidget->GetTypeFilterCount();

            m_typeFilters.reserve(numTypeFilters);

            for (int index = 0; index < numTypeFilters; index++)
            {
                m_typeFilters.append(aznew SavedTypeFilter());
            }
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

        void SearchAssetBrowserFavoriteItem::SetupFromSearchWidget(SearchWidget* searchWidget)
        {
            m_unusableProductsFilterActive = searchWidget->GetIsUnusableProductsFilterActive();
            m_folderFilterActive = searchWidget->GetIsFolderFilterActive();
            m_projectSourceFilterActive = searchWidget->GetIsProjectSourceAssetFilterActive();

            m_searchTerm = searchWidget->GetFilterString();

            int numTypeFilters = searchWidget->GetTypeFilterCount();

            for (int typeIndex = 0; typeIndex < numTypeFilters; typeIndex++)
            {
                SavedTypeFilter* typeFilter = m_typeFilters.at(typeIndex);
                searchWidget->GetTypeFilterDetails(typeIndex, typeFilter->categoryKey, typeFilter->displayName, typeFilter->enabled);
            }
        }

        void SearchAssetBrowserFavoriteItem::WriteToSearchWidget(SearchWidget* searchWidget)
        {
            searchWidget->ToggleUnusableProductsFilter(m_unusableProductsFilterActive);
            searchWidget->ToggleProjectSourceAssetFilter(m_projectSourceFilterActive);

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

        void SearchAssetBrowserFavoriteItem::LoadSettings(QSettings& settings)
        {
            m_name = settings.value("name").toString();
            m_searchTerm = settings.value("searchTerm").toString();
            m_unusableProductsFilterActive = settings.value("unusableProductsFilterActive").toBool();
            m_projectSourceFilterActive = settings.value("projectSourceFilterActive").toBool();
            m_folderFilterActive = settings.value("folderFilterActive").toBool();

            int filterCount = settings.beginReadArray("typeFilters");

            for (int index = 0; index < filterCount; index++)
            {
                SavedTypeFilter* typeFilter = m_typeFilters.at(index);

                typeFilter->categoryKey = settings.value("categoryKey").toString();
                typeFilter->displayName = settings.value("displayName").toString();
                typeFilter->enabled = settings.value("enabled").toBool();
            }

            settings.endArray();
        }

        void SearchAssetBrowserFavoriteItem::SaveSettings(QSettings& settings)
        {
            settings.setValue("name", m_name);
            settings.setValue("searchTerm", m_searchTerm);
            settings.setValue("unusableProductsFilterActive", m_unusableProductsFilterActive);
            settings.setValue("projectSourceFilterActive", m_projectSourceFilterActive);
            settings.setValue("folderFilterActive", m_folderFilterActive);

            settings.beginWriteArray("typeFilters", m_typeFilters.size());

            for (auto typeFilter : m_typeFilters)
            {
                settings.setValue("categoryKey", typeFilter->categoryKey);
                settings.setValue("displayName", typeFilter->displayName);
                settings.setValue("enabled", typeFilter->enabled);
            }

            settings.endArray();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

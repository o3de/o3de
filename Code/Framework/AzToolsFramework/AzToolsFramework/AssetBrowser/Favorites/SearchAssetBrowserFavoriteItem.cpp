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

        void SearchAssetBrowserFavoriteItem::LoadSettings(QSettings& settings)
        {
            m_name = settings.value("name").toString();
            m_searchTerm = settings.value("searchTerm").toString();
            m_unusableProductsFilterActive = settings.value("unusableProductsFilterActive").toBool();
            m_engineFilterActive = settings.value("projectSourceFilterActive").toBool();
            m_folderFilterActive = settings.value("folderFilterActive").toBool();

            int filterCount = settings.beginReadArray("typeFilters");

            qDeleteAll(m_typeFilters.begin(), m_typeFilters.end());
            m_typeFilters.clear();
            m_typeFilters.reserve(filterCount);

            for (int index = 0; index < filterCount; index++)
            {
                SavedTypeFilter* typeFilter = aznew SavedTypeFilter();
                
                typeFilter->categoryKey = settings.value("categoryKey").toString();
                typeFilter->displayName = settings.value("displayName").toString();
                typeFilter->enabled = settings.value("enabled").toBool();

                m_typeFilters.append(typeFilter);
            }

            settings.endArray();
        }

        void SearchAssetBrowserFavoriteItem::SaveSettings(QSettings& settings)
        {
            settings.setValue("name", m_name);
            settings.setValue("searchTerm", m_searchTerm);
            settings.setValue("unusableProductsFilterActive", m_unusableProductsFilterActive);
            settings.setValue("projectSourceFilterActive", m_engineFilterActive);
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesSettings.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        // ============================================================
        //  Reflection
        // ============================================================

        void FavoriteSearchTypeFilterRecord::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<FavoriteSearchTypeFilterRecord>()
                    ->Version(1)
                    ->Field("categoryKey", &FavoriteSearchTypeFilterRecord::m_categoryKey)
                    ->Field("displayName", &FavoriteSearchTypeFilterRecord::m_displayName)
                    ->Field("enabled", &FavoriteSearchTypeFilterRecord::m_enabled)
                    ;
            }
        }

        void FavoriteRecord::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<FavoriteRecord>()
                    ->Version(1)
                    ->Field("isSearch", &FavoriteRecord::m_isSearch)
                    ->Field("entryPath", &FavoriteRecord::m_entryPath)
                    ->Field("name", &FavoriteRecord::m_name)
                    ->Field("searchTerm", &FavoriteRecord::m_searchTerm)
                    ->Field("unusableProductsFilterActive", &FavoriteRecord::m_unusableProductsFilterActive)
                    ->Field("engineFilterActive", &FavoriteRecord::m_engineFilterActive)
                    ->Field("folderFilterActive", &FavoriteRecord::m_folderFilterActive)
                    ->Field("typeFilters", &FavoriteRecord::m_typeFilters)
                    ;
            }
        }

        void AssetBrowserFavoritesProjectSettings::Reflect(AZ::ReflectContext* context)
        {
            FavoriteSearchTypeFilterRecord::Reflect(context);
            FavoriteRecord::Reflect(context);

            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<AssetBrowserFavoritesProjectSettings>()
                    ->Version(1)
                    ->Field("items", &AssetBrowserFavoritesProjectSettings::m_items)
                    ->Field("unresolvedPaths", &AssetBrowserFavoritesProjectSettings::m_unresolvedPaths)
                    ;
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

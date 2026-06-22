/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        // ============================================================
        //  Persistent Records for Asset Browser Favorites
        // ============================================================
        //
        //  These records mirror the runtime AssetBrowserFavoriteItem
        //  hierarchy but in a form that can be round-tripped through
        //  AZ::SettingsRegistry::Set/GetObject and persisted in the
        //  per-project editorpreferences.setreg file.
        //
        //  This replaces the previous QSettings-based persistence,
        //  which was global to the machine and platform-specific.
        // ============================================================

        //! One enabled/disabled type filter as it appears on a saved search favorite.
        struct AZTF_API FavoriteSearchTypeFilterRecord
        {
            AZ_TYPE_INFO(FavoriteSearchTypeFilterRecord, "{2C7C3F1D-3D55-4D2A-A0B6-2F92E0E2C8AA}");
            AZ_CLASS_ALLOCATOR(FavoriteSearchTypeFilterRecord, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_categoryKey;
            AZStd::string m_displayName;
            bool m_enabled = false;
        };

        //! One favorite slot. Acts as a discriminated union -- m_isSearch
        //! selects which fields are meaningful (entry path vs. search filter).
        struct AZTF_API FavoriteRecord
        {
            AZ_TYPE_INFO(FavoriteRecord, "{6B79E0AF-3F5E-4D1A-9C1E-A8C61F6A38B1}");
            AZ_CLASS_ALLOCATOR(FavoriteRecord, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            // Discriminator
            bool m_isSearch = false;

            // Entry favorite (when !m_isSearch)
            AZStd::string m_entryPath;

            // Search favorite (when m_isSearch)
            AZStd::string m_name;
            AZStd::string m_searchTerm;
            bool m_unusableProductsFilterActive = false;
            bool m_engineFilterActive = false;
            bool m_folderFilterActive = false;
            AZStd::vector<FavoriteSearchTypeFilterRecord> m_typeFilters;
        };

        //! Top-level container persisted per project.
        //! m_unresolvedPaths preserves entry favorites whose target asset was
        //! not yet resolvable at load time, so they survive across sessions.
        struct AZTF_API AssetBrowserFavoritesProjectSettings
        {
            AZ_TYPE_INFO(AssetBrowserFavoritesProjectSettings, "{F1C9AB1D-7E51-44B2-B6D4-5DE7A5C76A12}");
            AZ_CLASS_ALLOCATOR(AssetBrowserFavoritesProjectSettings, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::vector<FavoriteRecord> m_items;
            AZStd::vector<AZStd::string> m_unresolvedPaths;
        };

        //! Settings registry path prefix where favorites live.
        //! Final path is: <prefix>/<projectName>
        constexpr const char* AssetBrowserFavoritesRegistryPrefix = "/O3DE/Preferences/AssetBrowser/Favorites";

    } // namespace AssetBrowser
} // namespace AzToolsFramework

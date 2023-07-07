/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h>

#include <QList>
#include <QString>
#include <QSettings>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SearchWidget;

        class SearchAssetBrowserFavoriteItem : public AssetBrowserFavoriteItem
        {
        public:
            struct SavedTypeFilter
            {
                QString categoryKey;
                QString displayName;
                bool enabled;
            };
            AZ_RTTI(SearchAssetBrowserFavoriteItem, "{1897792D-7ACD-4366-B813-C076C74FB316}");
            AZ_CLASS_ALLOCATOR(SearchAssetBrowserFavoriteItem, AZ::SystemAllocator);

            explicit SearchAssetBrowserFavoriteItem();
            ~SearchAssetBrowserFavoriteItem();

            void SetName(const QString name);
            const QString GetName() const;

            FavoriteType GetFavoriteType() const override;

            bool IsFilterActive();

            void SetupFromSearchWidget(SearchWidget* searchWidget);
            void WriteToSearchWidget(SearchWidget* searchWidget);

            void LoadSettings(QSettings& settings);
            void SaveSettings(QSettings& settings);

            QString GetDefaultName();
        private:
            QString m_name;
            QString m_searchTerm;
            bool m_unusableProductsFilterActive = false;
            bool m_engineFilterActive = false;
            bool m_folderFilterActive = false;
            QList<SavedTypeFilter*> m_typeFilters;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

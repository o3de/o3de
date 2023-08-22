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
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        class EntryAssetBrowserFavoriteItem
            : public AssetBrowserFavoriteItem
        {
        public:
            AZ_RTTI(EntryAssetBrowserFavoriteItem, "{BE4C2297-AA99-45AC-AB66-A8FD4010D237}");
            AZ_CLASS_ALLOCATOR(EntryAssetBrowserFavoriteItem, AZ::SystemAllocator);

            FavoriteType GetFavoriteType() const override;

            void SetEntry(const AssetBrowserEntry* entry);
            const AssetBrowserEntry* GetEntry() const;
        private:
            const AssetBrowserEntry* m_entry;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

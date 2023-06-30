/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h"
#include "AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h"

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserFavoriteItem::FavoriteType EntryAssetBrowserFavoriteItem::GetFavoriteType() const
        {
            return FavoriteType::AssetBrowserEntry;
        }

        void EntryAssetBrowserFavoriteItem::SetEntry(const AssetBrowserEntry* entry)
        {
            m_entry = entry;
        }

        const AssetBrowserEntry* EntryAssetBrowserFavoriteItem::GetEntry() const
        {
            return m_entry;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

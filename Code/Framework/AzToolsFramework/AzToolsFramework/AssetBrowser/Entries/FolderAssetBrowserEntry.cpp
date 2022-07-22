/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/FolderThumbnail.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserEntry::AssetEntryType FolderAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Folder;
        }

        void FolderAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_relativePath = m_relativePath / child->m_name;
            child->m_displayPath = QString::fromUtf8(child->m_relativePath.c_str());
            child->m_fullPath = m_fullPath / child->m_name;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey FolderAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(FolderThumbnailKey, m_fullPath.c_str());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

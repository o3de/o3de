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

        bool FolderAssetBrowserEntry::IsScanFolder() const
        {
            return m_isScanFolder;
        }

        void FolderAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            // the "relative paths" of a child is the path relative to a scan folder
            // thus, if we are a scan folder, the relative path is just the name of the child
            // but if we are not a scan folder, the relative path is our relative path + their name.
            if (IsScanFolder())
            {
                child->m_relativePath = child->m_name;
            }
            else
            {
                child->m_relativePath = (m_relativePath / child->m_name).LexicallyNormal();
            }

            // display path is just the relative path without the name:
            AZ::IO::Path parentPath = child->m_relativePath.ParentPath();
            child->m_displayPath = QString::fromUtf8(parentPath.c_str());
            child->m_fullPath = (m_fullPath / child->m_name).LexicallyNormal();
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey FolderAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(FolderThumbnailKey, m_fullPath.c_str());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

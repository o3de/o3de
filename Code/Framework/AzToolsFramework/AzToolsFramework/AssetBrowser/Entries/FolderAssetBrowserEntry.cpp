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
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        FolderAssetBrowserEntry::FolderAssetBrowserEntry()
            : m_folderUuid(AZ::Uuid::CreateRandom())
        {
            EntryCache::GetInstance()->m_folderUuidMap[m_folderUuid] = this;
        }

        FolderAssetBrowserEntry::~FolderAssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_folderUuidMap.erase(m_folderUuid);
            }
        }

        AssetBrowserEntry::AssetEntryType FolderAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Folder;
        }

        bool FolderAssetBrowserEntry::IsScanFolder() const
        {
            return m_isScanFolder;
        }

        bool FolderAssetBrowserEntry::IsGemFolder() const
        {
            return m_isGemFolder;
        }

        const AZ::Uuid& FolderAssetBrowserEntry::GetFolderUuid() const
        {
            return m_folderUuid;
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

            // the visible path of a child is the path that is visible in the asset browser
            child->m_visiblePath = (m_visiblePath / child->m_name).LexicallyNormal();

            // display path is just the relative path without the name:
            AZ::IO::Path parentPath = child->m_relativePath.ParentPath();
            child->m_displayPath = QString::fromUtf8(parentPath.c_str());
            child->SetFullPath((m_fullPath / child->m_name).LexicallyNormal());
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey FolderAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(FolderThumbnailKey, m_fullPath.c_str());
        }

        const FolderAssetBrowserEntry* FolderAssetBrowserEntry::GetFolderByUuid(const AZ::Uuid& folderUuid)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                return cache->m_folderUuidMap[folderUuid];
            }
            return nullptr;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

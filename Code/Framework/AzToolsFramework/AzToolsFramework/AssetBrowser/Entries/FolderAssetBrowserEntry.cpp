/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        void FolderAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<FolderAssetBrowserEntry, AssetBrowserEntry>()
                    ->Field("m_isGemsFolder", &FolderAssetBrowserEntry::m_isGemsFolder)
                    ->Version(1);
            }
        }

        AssetBrowserEntry::AssetEntryType FolderAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Folder;
        }

        bool FolderAssetBrowserEntry::IsGemsFolder() const
        {
            return m_isGemsFolder;
        }

        void FolderAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_relativePath = m_relativePath + AZ_CORRECT_DATABASE_SEPARATOR + child->m_name;
            child->m_fullPath = m_fullPath + AZ_CORRECT_DATABASE_SEPARATOR + child->m_name;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey FolderAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(FolderThumbnailKey, m_fullPath.c_str(), IsGemsFolder());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

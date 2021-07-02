/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/FolderThumbnail.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        FolderThumbnailKey::FolderThumbnailKey(const char* folderPath, bool isGem)
            : ThumbnailKey()
            , m_folderPath(folderPath)
            , m_isGem(isGem)
        {}

        const AZStd::string& FolderThumbnailKey::GetFolderPath() const
        {
            return m_folderPath;
        }

        bool FolderThumbnailKey::IsGem() const
        {
            return m_isGem;
        }

        bool FolderThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            if (!ThumbnailKey::Equals(other))
            {
                return false;
            }
            return m_isGem == azrtti_cast<const FolderThumbnailKey*>(other)->IsGem();
        }
        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnail
        //////////////////////////////////////////////////////////////////////////
        static constexpr const char* FolderIconPath = "Assets/Editor/Icons/AssetBrowser/Folder_16.svg";
        static constexpr const char* GemIconPath = "Assets/Editor/Icons/AssetBrowser/GemFolder_16.svg";

        FolderThumbnail::FolderThumbnail(SharedThumbnailKey key)
            : Thumbnail(key)
        {}

        void FolderThumbnail::LoadThread()
        {
            auto folderKey = azrtti_cast<const FolderThumbnailKey*>(m_key.data());
            AZ_Assert(folderKey, "Incorrect key type, excpected FolderThumbnailKey");

            const char* folderIcon = folderKey->IsGem() ? GemIconPath : FolderIconPath;
            auto absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / folderIcon;
            m_pixmap.load(absoluteIconPath.c_str());
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
        }

        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        FolderThumbnailCache::FolderThumbnailCache()
            : ThumbnailCache<FolderThumbnail>() {}

        FolderThumbnailCache::~FolderThumbnailCache() = default;

        const char* FolderThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool FolderThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const FolderThumbnailKey*>(key.data());
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Thumbnails/moc_FolderThumbnail.cpp"

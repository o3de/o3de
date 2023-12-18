/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        FolderThumbnailKey::FolderThumbnailKey(const char* folderPath)
            : ThumbnailKey()
            , m_folderPath(folderPath)
        {}

        const AZStd::string& FolderThumbnailKey::GetFolderPath() const
        {
            return m_folderPath;
        }

        bool FolderThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            return ThumbnailKey::Equals(other);
        }
        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnail
        //////////////////////////////////////////////////////////////////////////
        static constexpr const char* FolderIconPath = "Assets/Editor/Icons/AssetBrowser/Folder_80.svg";

        FolderThumbnail::FolderThumbnail(SharedThumbnailKey key)
            : Thumbnail(key)
        {}

        void FolderThumbnail::Load()
        {
            m_state = State::Loading;
            AZ_Assert(azrtti_cast<const FolderThumbnailKey*>(m_key.data()), "Incorrect key type, excpected FolderThumbnailKey");

            auto absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / FolderIconPath;
            m_pixmap.load(absoluteIconPath.c_str());
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
            QueueThumbnailUpdated();
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

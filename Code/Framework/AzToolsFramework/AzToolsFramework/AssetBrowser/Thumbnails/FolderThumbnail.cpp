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

#include <AzCore/Debug/Trace.h>
#include <AzCore/StringFunc/StringFunc.h>
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
        static constexpr const char* FolderIconPath = "Icons/AssetBrowser/Folder_16.svg";
        static constexpr const char* GemIconPath = "Icons/AssetBrowser/GemFolder_16.svg";

        FolderThumbnail::FolderThumbnail(SharedThumbnailKey key)
            : Thumbnail(key)
        {}

        void FolderThumbnail::LoadThread()
        {
            auto folderKey = azrtti_cast<const FolderThumbnailKey*>(m_key.data());
            AZ_Assert(folderKey, "Incorrect key type, excpected FolderThumbnailKey");

            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            AZ_Assert(engineRoot, "Engine Root not initialized");
            const char* folderIcon = folderKey->IsGem() ? GemIconPath : FolderIconPath;
            AZStd::string absoluteIconPath;
            AZ::StringFunc::Path::Join(engineRoot, folderIcon, absoluteIconPath);

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

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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QObject>
#endif

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetBrowser
    {
        //! FolderAssetBrowserEntry thumbnail key
        class FolderThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            AZ_RTTI(FolderThumbnailKey, "{47B5423B-1324-46AD-BBA9-791D5C4116B5}", ThumbnailKey);

            FolderThumbnailKey(const char* folderPath, bool isGem);
            const AZStd::string& GetFolderPath() const;
            bool IsGem() const;
            bool Equals(const ThumbnailKey* other) const override;

        protected:
            //! Absolute folder path
            AZStd::string m_folderPath;
            //! is folder a gem
            bool m_isGem;
        };

        class FolderThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            FolderThumbnail(SharedThumbnailKey key, int thumbnailSize);
            void LoadThread() override;
        };


        //! FolderAssetBrowserEntry thumbnails
        class FolderThumbnailCache
            : public ThumbnailCache<FolderThumbnail>
        {
        public:
            FolderThumbnailCache();
            ~FolderThumbnailCache() override;

            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Folder Thumbnails";

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

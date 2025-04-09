/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            FolderThumbnailKey(const char* folderPath);
            const AZStd::string& GetFolderPath() const;
            bool IsGem() const;
            bool Equals(const ThumbnailKey* other) const override;

        protected:
            //! Absolute folder path
            AZStd::string m_folderPath;
        };

        class FolderThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            FolderThumbnail(SharedThumbnailKey key);
            void Load() override;
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

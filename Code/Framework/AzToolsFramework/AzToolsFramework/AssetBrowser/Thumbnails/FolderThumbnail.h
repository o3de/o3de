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

        namespace
        {
            class FolderKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& /*val*/) const
                {
                    return 0;
                }
            };

            class FolderKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto folderThumbnailKey1 = azrtti_cast<const FolderThumbnailKey*>(val1.data());
                    auto folderThumbnailKey2 = azrtti_cast<const FolderThumbnailKey*>(val2.data());
                    if (!folderThumbnailKey1 || !folderThumbnailKey2)
                    {
                        return false;
                    }
                    // There are only two thumbnails in this cache, one for gem icon and one for folder icon
                    return folderThumbnailKey1->IsGem() == folderThumbnailKey2->IsGem();
                }
            };
        }
        //! FolderAssetBrowserEntry thumbnails
        class FolderThumbnailCache
            : public ThumbnailCache<FolderThumbnail, FolderKeyHash, FolderKeyEqual>
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

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
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductThumbnailKey
            : public Thumbnailer::ThumbnailKey
        {
            Q_OBJECT
        public:
            AZ_RTTI(ProductThumbnailKey, "{00FED34F-3D86-41B0-802F-9BD218DA1F0D}", Thumbnailer::ThumbnailKey);

            explicit ProductThumbnailKey(const AZ::Data::AssetId& assetId);
            const AZ::Data::AssetId& GetAssetId() const;
            const AZ::Data::AssetType& GetAssetType() const;
            size_t GetHash() const override;
            bool Equals(const ThumbnailKey* other) const override;

        protected:
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;
        };

        class ProductThumbnail
            : public Thumbnailer::Thumbnail
        {
            Q_OBJECT
        public:
            ProductThumbnail(Thumbnailer::SharedThumbnailKey key);

        protected:
            void LoadThread() override;
        };

        //! ProductAssetBrowserEntry thumbnails
        class ProductThumbnailCache
            : public Thumbnailer::ThumbnailCache<ProductThumbnail>
        {
        public:
            ProductThumbnailCache();
            ~ProductThumbnailCache() override;

            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Product Thumbnails";

        protected:
            bool IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

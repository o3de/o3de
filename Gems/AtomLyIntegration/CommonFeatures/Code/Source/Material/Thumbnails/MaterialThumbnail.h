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
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>

#include <Material/Thumbnails/MaterialThumbnailRenderer.h>
#endif

namespace AZ
{
    namespace LyIntegration
    {
        /**
         * Custom material thumbnail that detects when a material asset changes and updates the thumbnail
         */
        class MaterialThumbnail
            : public AzToolsFramework::Thumbnailer::Thumbnail
            , public AzToolsFramework::ThumbnailerRendererNotificationBus::Handler
            , private AzFramework::AssetCatalogEventBus::Handler
        {
        Q_OBJECT
        public:
            MaterialThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, int thumbnailSize);
            ~MaterialThumbnail() override;

            //! AzToolsFramework::ThumbnailerRendererNotificationBus::Handler overrides...
            void ThumbnailRendered(QPixmap& thumbnailImage) override;
            void ThumbnailFailedToRender() override;

        protected:
            void LoadThread() override;

        private:
            // AzFramework::AssetCatalogEventBus::Handler interface overrides...
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

            Data::AssetId m_assetId;
            Data::AssetType m_assetType;
            AZStd::binary_semaphore m_renderWait;
        };


        /**
         * Cache configuration for large material thumbnails
         */
        class MaterialThumbnailCache
            : public AzToolsFramework::Thumbnailer::ThumbnailCache<MaterialThumbnail, AzToolsFramework::AssetBrowser::SourceKeyHash, AzToolsFramework::AssetBrowser::SourceKeyEqual>
        {
        public:
            MaterialThumbnailCache();
            ~MaterialThumbnailCache() override;

            int GetPriority() const override;
            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Material Thumbnails";

        protected:
            bool IsSupportedThumbnail(AzToolsFramework::SharedThumbnailKey key) const override;

        private:
            AZStd::unique_ptr<MaterialThumbnailRenderer> m_renderer;
        };
    } // namespace LyIntegration
} // namespace AZ

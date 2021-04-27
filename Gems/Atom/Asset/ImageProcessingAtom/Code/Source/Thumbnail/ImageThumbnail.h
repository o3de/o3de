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
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#endif

namespace ImageProcessingAtom
{
    namespace Thumbnails
    {
        /**
         * Custom image thumbnail that detects when an asset changes and updates the thumbnail
         */
        class ImageThumbnail
            : public AzToolsFramework::Thumbnailer::Thumbnail
            , public AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
        {
            Q_OBJECT
        public:
            ImageThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, int thumbnailSize);
            ~ImageThumbnail() override;

            //! AzToolsFramework::ThumbnailerRendererNotificationBus::Handler overrides...
            void ThumbnailRendered(QPixmap& thumbnailImage) override;
            void ThumbnailFailedToRender() override;

        protected:
            void LoadThread() override;

        private:
            // AzFramework::AssetCatalogEventBus::Handler interface overrides...
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

            AZStd::binary_semaphore m_renderWait;
            AZStd::unordered_set<AZ::Data::AssetId> m_assetIds;
        };

        /**
         * Cache configuration for large image thumbnails
         */
        class ImageThumbnailCache
            : public AzToolsFramework::Thumbnailer::ThumbnailCache<ImageThumbnail>
        {
        public:
            ImageThumbnailCache();
            ~ImageThumbnailCache() override;

            int GetPriority() const override;
            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Image Thumbnails";

        protected:
            bool IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const override;
        };
    } // namespace Thumbnails
} // namespace ImageProcessingAtom

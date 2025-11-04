/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#endif

namespace AZ
{
    namespace LyIntegration
    {
        //! Custom thumbnail for most common Atom assets
        //! Detects asset changes and updates the thumbnail
        class SharedThumbnail final
            : public AzToolsFramework::Thumbnailer::Thumbnail
            , public AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler
            , private AzFramework::AssetCatalogEventBus::Handler
        {
            Q_OBJECT
        public:
            SharedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key);
            ~SharedThumbnail() override;

            //! AzToolsFramework::ThumbnailerRendererNotificationBus::Handler overrides...
            void ThumbnailRendered(const QPixmap& thumbnailImage) override;
            void ThumbnailFailedToRender() override;
            void Load() override;

        private:
            // AzFramework::AssetCatalogEventBus::Handler interface overrides...
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

            Data::AssetInfo m_assetInfo;
        };

        //! Cache configuration for shared thumbnails
        class SharedThumbnailCache final : public AzToolsFramework::Thumbnailer::ThumbnailCache<SharedThumbnail>
        {
        public:
            SharedThumbnailCache();
            ~SharedThumbnailCache() override;

            int GetPriority() const override;
            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Common Feature Shared Thumbnail Provider";

        protected:
            bool IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const override;
        };
    } // namespace LyIntegration
} // namespace AZ

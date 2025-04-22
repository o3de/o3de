/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <SharedPreview/SharedPreviewUtils.h>
#include <SharedPreview/SharedThumbnail.h>

namespace AZ
{
    namespace LyIntegration
    {
        static constexpr const int SharedThumbnailSize = 256;

        //////////////////////////////////////////////////////////////////////////
        // SharedThumbnail
        //////////////////////////////////////////////////////////////////////////
        SharedThumbnail::SharedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            : Thumbnail(key)
            , m_assetInfo(SharedPreviewUtils::GetSupportedAssetInfo(key))
        {
            if (m_assetInfo.m_assetId.IsValid())
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(m_key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
                return;
            }

            AZ_Error("SharedThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
            m_state = State::Failed;
        }

        SharedThumbnail::~SharedThumbnail()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }

        void SharedThumbnail::Load()
        {
            m_state = State::Loading;
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                m_assetInfo.m_assetType, &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail, m_key,
                SharedThumbnailSize);
        }

        void SharedThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_state = State::Ready;
            QueueThumbnailUpdated();
        }

        void SharedThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            QueueThumbnailUpdated();
        }

        void SharedThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
        {
            if (m_assetInfo.m_assetId == assetId && (m_state == State::Ready || m_state == State::Failed))
            {
                m_state = State::Unloaded;
                Load();
            }
        }

        void SharedThumbnail::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& /*assetInfo*/)
        {
            if (m_assetInfo.m_assetId == assetId)
            {
                // Removing the thumbnail from the catalog asset doesn't remove it from the thumbnail cache.
                // Therefore marking the state as unloaded ensures that a new pixmap will be rendered on the next access.
                m_state = State::Unloaded;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // SharedThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SharedThumbnailCache::SharedThumbnailCache()
            : ThumbnailCache<SharedThumbnail>()
        {
        }

        SharedThumbnailCache::~SharedThumbnailCache() = default;

        int SharedThumbnailCache::GetPriority() const
        {
            // Custom thumbnails have a higher priority to override default source thumbnails
            return 1;
        }

        const char* SharedThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool SharedThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
        {
            return SharedPreviewUtils::IsSupportedAssetType(key);
        }
    } // namespace LyIntegration
} // namespace AZ

#include <SharedPreview/moc_SharedThumbnail.cpp>

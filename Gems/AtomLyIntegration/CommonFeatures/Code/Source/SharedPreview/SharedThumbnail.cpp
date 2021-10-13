/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <SharedPreview/SharedThumbnail.h>
#include <SharedPreview/SharedPreviewUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int SharedThumbnailSize = 512; // 512 is the default size in render to texture pass

            //////////////////////////////////////////////////////////////////////////
            // SharedThumbnail
            //////////////////////////////////////////////////////////////////////////
            SharedThumbnail::SharedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                m_assetId = GetAssetId(key, RPI::MaterialAsset::RTTI_Type());
                if (!m_assetId.IsValid())
                {
                    AZ_Error("SharedThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                    m_state = State::Failed;
                    return;
                }

                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }

            void SharedThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    RPI::MaterialAsset::RTTI_Type(),
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail,
                    m_key,
                    SharedThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            SharedThumbnail::~SharedThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void SharedThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void SharedThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void SharedThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId &&
                    m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
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
                // Material thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* SharedThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool SharedThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
            {
                return
                    GetAssetId(key, RPI::MaterialAsset::RTTI_Type()).IsValid() &&
                    // in case it's a source scene file, it will contain both material and model products
                    // model thumbnails are handled by MeshThumbnail
                    !GetAssetId(key, RPI::ModelAsset::RTTI_Type()).IsValid();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <SharedPreview/moc_SharedThumbnail.cpp>

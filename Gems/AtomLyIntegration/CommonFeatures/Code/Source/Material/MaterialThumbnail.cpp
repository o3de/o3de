/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <Source/Material/MaterialThumbnail.h>
#include <Source/Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int MaterialThumbnailSize = 512; // 512 is the default size in render to texture pass

            //////////////////////////////////////////////////////////////////////////
            // MaterialThumbnail
            //////////////////////////////////////////////////////////////////////////
            MaterialThumbnail::MaterialThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                m_assetId = GetAssetId(key, RPI::MaterialAsset::RTTI_Type());
                if (!m_assetId.IsValid())
                {
                    AZ_Error("MaterialThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                    m_state = State::Failed;
                    return;
                }

                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }

            void MaterialThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    RPI::MaterialAsset::RTTI_Type(),
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail,
                    m_key,
                    MaterialThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            MaterialThumbnail::~MaterialThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void MaterialThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void MaterialThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void MaterialThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId &&
                    m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // MaterialThumbnailCache
            //////////////////////////////////////////////////////////////////////////
            MaterialThumbnailCache::MaterialThumbnailCache()
                : ThumbnailCache<MaterialThumbnail>()
            {
            }

            MaterialThumbnailCache::~MaterialThumbnailCache() = default;

            int MaterialThumbnailCache::GetPriority() const
            {
                // Material thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* MaterialThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool MaterialThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
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

#include <Material/moc_MaterialThumbnail.cpp>

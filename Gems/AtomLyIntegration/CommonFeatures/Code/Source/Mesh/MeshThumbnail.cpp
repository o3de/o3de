/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <QtConcurrent/QtConcurrent>
#include <Source/Mesh/MeshThumbnail.h>
#include <Source/Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int MeshThumbnailSize = 512; // 512 is the default size in render to texture pass

            //////////////////////////////////////////////////////////////////////////
            // MeshThumbnail
            //////////////////////////////////////////////////////////////////////////
            MeshThumbnail::MeshThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                m_assetId = GetAssetId(key, RPI::ModelAsset::RTTI_Type());
                if (!m_assetId.IsValid())
                {
                    AZ_Error("MeshThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                    m_state = State::Failed;
                    return;
                }

                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }

            void MeshThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    RPI::ModelAsset::RTTI_Type(),
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail,
                    m_key,
                    MeshThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            MeshThumbnail::~MeshThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void MeshThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void MeshThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void MeshThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId &&
                    m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // MeshThumbnailCache
            //////////////////////////////////////////////////////////////////////////
            MeshThumbnailCache::MeshThumbnailCache()
                : ThumbnailCache<MeshThumbnail>()
            {
            }

            MeshThumbnailCache::~MeshThumbnailCache() = default;

            int MeshThumbnailCache::GetPriority() const
            {
                // Material thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* MeshThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool MeshThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
            {
                return GetAssetId(key, RPI::ModelAsset::RTTI_Type()).IsValid();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <Mesh/moc_MeshThumbnail.cpp>

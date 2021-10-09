/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <Previewer/ModelThumbnail.h>
#include <Previewer/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int ModelThumbnailSize = 512; // 512 is the default size in render to texture pass

            //////////////////////////////////////////////////////////////////////////
            // ModelThumbnail
            //////////////////////////////////////////////////////////////////////////
            ModelThumbnail::ModelThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                m_assetId = GetAssetId(key, RPI::ModelAsset::RTTI_Type());
                if (!m_assetId.IsValid())
                {
                    AZ_Error("ModelThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                    m_state = State::Failed;
                    return;
                }

                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }

            void ModelThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    RPI::ModelAsset::RTTI_Type(), &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail, m_key,
                    ModelThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            ModelThumbnail::~ModelThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void ModelThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void ModelThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void ModelThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId && m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // ModelThumbnailCache
            //////////////////////////////////////////////////////////////////////////
            ModelThumbnailCache::ModelThumbnailCache()
                : ThumbnailCache<ModelThumbnail>()
            {
            }

            ModelThumbnailCache::~ModelThumbnailCache() = default;

            int ModelThumbnailCache::GetPriority() const
            {
                // Thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* ModelThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool ModelThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
            {
                return GetAssetId(key, RPI::ModelAsset::RTTI_Type()).IsValid();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <Previewer/moc_ModelThumbnail.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Previewer/CommonThumbnail.h>
#include <Previewer/CommonThumbnailUtils.h>
#include <QtConcurrent/QtConcurrent>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int CommonThumbnailSize = 256;

            //////////////////////////////////////////////////////////////////////////
            // CommonThumbnail
            //////////////////////////////////////////////////////////////////////////
            CommonThumbnail::CommonThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                for (const AZ::Uuid& typeId : GetSupportedThumbnailAssetTypes())
                {
                    const AZ::Data::AssetId& assetId = GetAssetId(key, typeId);
                    if (assetId.IsValid())
                    {
                        m_assetId = assetId;
                        m_typeId = typeId;
                        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
                        return;
                    }
                }

                AZ_Error("CommonThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                m_state = State::Failed;
            }

            void CommonThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    m_typeId, &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail, m_key,
                    CommonThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            CommonThumbnail::~CommonThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void CommonThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void CommonThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void CommonThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId && m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // CommonThumbnailCache
            //////////////////////////////////////////////////////////////////////////
            CommonThumbnailCache::CommonThumbnailCache()
                : ThumbnailCache<CommonThumbnail>()
            {
            }

            CommonThumbnailCache::~CommonThumbnailCache() = default;

            int CommonThumbnailCache::GetPriority() const
            {
                // Thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* CommonThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool CommonThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
            {
                return Thumbnails::IsSupportedThumbnail(key);
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <Previewer/moc_CommonThumbnail.cpp>

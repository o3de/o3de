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
            const int MaterialThumbnailSize = 200;

            //////////////////////////////////////////////////////////////////////////
            // MaterialThumbnail
            //////////////////////////////////////////////////////////////////////////
            MaterialThumbnail::MaterialThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, int thumbnailSize)
                : Thumbnail(key, thumbnailSize)
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
                    m_thumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            MaterialThumbnail::~MaterialThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void MaterialThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
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
                    // in case it's a source fbx, it will contain both material and model products
                    // model thumbnails are handled by MeshThumbnail
                    !GetAssetId(key, RPI::ModelAsset::RTTI_Type()).IsValid();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <Material/moc_MaterialThumbnail.cpp>

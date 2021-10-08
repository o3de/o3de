/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <Thumbnails/LightingPresetThumbnail.h>
#include <Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            static constexpr const int LightingPresetThumbnailSize = 512; // 512 is the default size in render to texture pass

            //////////////////////////////////////////////////////////////////////////
            // LightingPresetThumbnail
            //////////////////////////////////////////////////////////////////////////
            LightingPresetThumbnail::LightingPresetThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
                : Thumbnail(key)
            {
                m_assetId = GetAssetId(key, RPI::AnyAsset::RTTI_Type());
                if (!m_assetId.IsValid())
                {
                    AZ_Error("LightingPresetThumbnail", false, "Failed to find matching assetId for the thumbnailKey.");
                    m_state = State::Failed;
                    return;
                }

                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
                AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            }

            void LightingPresetThumbnail::LoadThread()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                    RPI::AnyAsset::RTTI_Type(), &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail, m_key,
                    LightingPresetThumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }

            LightingPresetThumbnail::~LightingPresetThumbnail()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }

            void LightingPresetThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
            {
                m_pixmap = thumbnailImage;
                m_renderWait.release();
            }

            void LightingPresetThumbnail::ThumbnailFailedToRender()
            {
                m_state = State::Failed;
                m_renderWait.release();
            }

            void LightingPresetThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
                if (m_assetId == assetId && m_state == State::Ready)
                {
                    m_state = State::Unloaded;
                    Load();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // LightingPresetThumbnailCache
            //////////////////////////////////////////////////////////////////////////
            LightingPresetThumbnailCache::LightingPresetThumbnailCache()
                : ThumbnailCache<LightingPresetThumbnail>()
            {
            }

            LightingPresetThumbnailCache::~LightingPresetThumbnailCache() = default;

            int LightingPresetThumbnailCache::GetPriority() const
            {
                // Thumbnails override default source thumbnails, so carry higher priority
                return 1;
            }

            const char* LightingPresetThumbnailCache::GetProviderName() const
            {
                return ProviderName;
            }

            bool LightingPresetThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
            {
                const auto assetId = Thumbnails::GetAssetId(key, RPI::AnyAsset::RTTI_Type());
                if (assetId.IsValid())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                    return AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), "lightingpreset.azasset");
                }

                return false;
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

#include <Thumbnails/moc_LightingPresetThumbnail.cpp>

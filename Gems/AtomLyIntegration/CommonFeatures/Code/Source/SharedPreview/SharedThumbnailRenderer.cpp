/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Previewer/CommonPreviewContent.h>
#include <Previewer/CommonThumbnailRenderer.h>
#include <Previewer/CommonThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonThumbnailRenderer::CommonThumbnailRenderer()
            {
                m_previewRenderer.reset(aznew AtomToolsFramework::PreviewRenderer(
                    "CommonThumbnailRenderer Preview Scene", "CommonThumbnailRenderer Preview Pipeline"));

                m_defaultModelAsset.Create(DefaultModelAssetId, true);
                m_defaultMaterialAsset.Create(DefaultMaterialAssetId, true);
                m_defaultLightingPresetAsset.Create(DefaultLightingPresetAssetId, true);

                for (const AZ::Uuid& typeId : GetSupportedThumbnailAssetTypes())
                {
                    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(typeId);
                }
                SystemTickBus::Handler::BusConnect();
            }

            CommonThumbnailRenderer::~CommonThumbnailRenderer()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
            }

            void CommonThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                m_previewRenderer->AddCaptureRequest(
                    { thumbnailSize,
                      AZStd::make_shared<CommonPreviewContent>(
                          m_previewRenderer->GetScene(), m_previewRenderer->GetView(), m_previewRenderer->GetEntityContextId(),
                          GetAssetId(thumbnailKey, RPI::ModelAsset::RTTI_Type(), DefaultModelAssetId),
                          GetAssetId(thumbnailKey, RPI::MaterialAsset::RTTI_Type(), DefaultMaterialAssetId),
                          GetAssetId(thumbnailKey, RPI::AnyAsset::RTTI_Type(), DefaultLightingPresetAssetId),
                          Render::MaterialPropertyOverrideMap()),
                      [thumbnailKey]()
                      {
                          AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                              thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                      },
                      [thumbnailKey](const QPixmap& pixmap)
                      {
                          AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                              thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered, pixmap);
                      } });
            }

            bool CommonThumbnailRenderer::Installed() const
            {
                return true;
            }

            void CommonThumbnailRenderer::OnSystemTick()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

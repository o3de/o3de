/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/CommonThumbnailPreviewContent.h>
#include <Thumbnails/CommonThumbnailRenderer.h>
#include <Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonThumbnailRenderer::CommonThumbnailRenderer()
            {
                // CommonThumbnailRenderer supports both models and materials
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::MaterialAsset::RTTI_Type());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::ModelAsset::RTTI_Type());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::AnyAsset::RTTI_Type());
                SystemTickBus::Handler::BusConnect();
            }

            CommonThumbnailRenderer::~CommonThumbnailRenderer()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
            }

            void CommonThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                m_previewRenderer.AddCaptureRequest(
                    { thumbnailSize,
                      AZStd::make_shared<CommonThumbnailPreviewContent>(
                          m_previewRenderer.GetScene(),
                          m_previewRenderer.GetView(),
                          m_previewRenderer.GetEntityContextId(),
                          GetAssetId(thumbnailKey, RPI::ModelAsset::RTTI_Type()),
                          GetAssetId(thumbnailKey, RPI::MaterialAsset::RTTI_Type()),
                          GetAssetId(thumbnailKey, RPI::AnyAsset::RTTI_Type())),
                          [thumbnailKey]()
                          {
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                          },
                          [thumbnailKey](const QImage& image)
                          {
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                                  QPixmap::fromImage(image));
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

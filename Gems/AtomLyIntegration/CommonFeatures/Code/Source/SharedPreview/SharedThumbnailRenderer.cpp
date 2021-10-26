/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <SharedPreview/SharedPreviewContent.h>
#include <SharedPreview/SharedPreviewUtils.h>
#include <SharedPreview/SharedThumbnailRenderer.h>

namespace AZ
{
    namespace LyIntegration
    {
        SharedThumbnailRenderer::SharedThumbnailRenderer()
        {
            m_defaultModelAsset.Create(DefaultModelAssetId, true);
            m_defaultMaterialAsset.Create(DefaultMaterialAssetId, true);
            m_defaultLightingPresetAsset.Create(DefaultLightingPresetAssetId, true);

            for (const AZ::Uuid& typeId : SharedPreviewUtils::GetSupportedAssetTypes())
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(typeId);
            }
            SystemTickBus::Handler::BusConnect();
        }

        SharedThumbnailRenderer::~SharedThumbnailRenderer()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
            SystemTickBus::Handler::BusDisconnect();
        }

        void SharedThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
        {
            if (auto previewRenderer = AZ::Interface<AtomToolsFramework::PreviewRendererInterface>::Get())
            {
                previewRenderer->AddCaptureRequest(
                    { thumbnailSize,
                      AZStd::make_shared<SharedPreviewContent>(
                          previewRenderer->GetScene(), previewRenderer->GetView(), previewRenderer->GetEntityContextId(),
                          SharedPreviewUtils::GetAssetId(thumbnailKey, RPI::ModelAsset::RTTI_Type(), DefaultModelAssetId),
                          SharedPreviewUtils::GetAssetId(thumbnailKey, RPI::MaterialAsset::RTTI_Type(), DefaultMaterialAssetId),
                          SharedPreviewUtils::GetAssetId(thumbnailKey, RPI::AnyAsset::RTTI_Type(), DefaultLightingPresetAssetId),
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
        }

        bool SharedThumbnailRenderer::Installed() const
        {
            return true;
        }

        void SharedThumbnailRenderer::OnSystemTick()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
        }
    } // namespace LyIntegration
} // namespace AZ

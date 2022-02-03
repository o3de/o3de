/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererInterface.h>
#include <AtomToolsFramework/Util/Util.h>
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
            m_defaultModelAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(DefaultModelPath), true);
            m_defaultMaterialAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(DefaultMaterialPath), true);
            m_defaultLightingPresetAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(DefaultLightingPresetPath), true);

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

        SharedThumbnailRenderer::ThumbnailConfig SharedThumbnailRenderer::GetThumbnailConfig(
            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey)
        {
            ThumbnailConfig thumbnailConfig;

            const auto assetInfo = SharedPreviewUtils::GetSupportedAssetInfo(thumbnailKey);
            if (assetInfo.m_assetType == RPI::ModelAsset::RTTI_Type())
            {
                static constexpr const char* MaterialAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/MaterialAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/LightingAssetPath";

                thumbnailConfig.m_modelId = assetInfo.m_assetId;
                thumbnailConfig.m_materialId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(MaterialAssetPathSetting, DefaultMaterialPath));
                thumbnailConfig.m_lightingId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath));
            }
            else if (assetInfo.m_assetType == RPI::MaterialAsset::RTTI_Type())
            {
                static constexpr const char* ModelAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/ModelAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/LightingAssetPath";

                thumbnailConfig.m_modelId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(ModelAssetPathSetting, DefaultModelPath));
                thumbnailConfig.m_materialId = assetInfo.m_assetId;
                thumbnailConfig.m_lightingId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath));
            }
            else if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type())
            {
                static constexpr const char* ModelAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingAssetType/ModelAssetPath";
                static constexpr const char* MaterialAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingAssetType/MaterialAssetPath";

                thumbnailConfig.m_modelId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(ModelAssetPathSetting, DefaultModelPath));
                thumbnailConfig.m_materialId = SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingOrDefault<AZStd::string>(MaterialAssetPathSetting, "materials/reflectionprobe/reflectionprobevisualization.azmaterial"));
                thumbnailConfig.m_lightingId = assetInfo.m_assetId;
            }

            return thumbnailConfig;
        }

        void SharedThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
        {
            if (auto previewRenderer = AZ::Interface<AtomToolsFramework::PreviewRendererInterface>::Get())
            {
                const auto& thumbnailConfig = GetThumbnailConfig(thumbnailKey);

                previewRenderer->AddCaptureRequest(
                    { thumbnailSize,
                      AZStd::make_shared<SharedPreviewContent>(
                          previewRenderer->GetScene(), previewRenderer->GetView(), previewRenderer->GetEntityContextId(),
                          thumbnailConfig.m_modelId, thumbnailConfig.m_materialId, thumbnailConfig.m_lightingId,
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
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
            const auto assetInfo = SharedPreviewUtils::GetSupportedAssetInfo(thumbnailKey);
            if (assetInfo.m_assetType == RPI::ModelAsset::RTTI_Type())
            {
                static constexpr const char* MaterialAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/MaterialAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/LightingAssetPath";

                ThumbnailConfig thumbnailConfig;
                thumbnailConfig.m_modelAsset.Create(assetInfo.m_assetId);
                thumbnailConfig.m_materialAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(MaterialAssetPathSetting, DefaultMaterialPath)));
                thumbnailConfig.m_lightingAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath)));
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::MaterialAsset::RTTI_Type())
            {
                static constexpr const char* ModelAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/ModelAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/LightingAssetPath";

                ThumbnailConfig thumbnailConfig;
                thumbnailConfig.m_modelAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(ModelAssetPathSetting, DefaultModelPath)));
                thumbnailConfig.m_materialAsset.Create(assetInfo.m_assetId);
                thumbnailConfig.m_lightingAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath)));
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::MaterialTypeAsset::RTTI_Type())
            {
                static constexpr const char* ModelAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialTypeAssetType/ModelAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialTypeAssetType/LightingAssetPath";

                // Material type assets are not renderable so we generate a material asset from the material type. 
                ThumbnailConfig thumbnailConfig;
                AZ::RPI::MaterialSourceData materialSourceData;
                materialSourceData.m_materialType = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                auto outcome = materialSourceData.CreateMaterialAsset(
                    AZ::Uuid::CreateRandom(), "", AZ::RPI::MaterialAssetProcessingMode::PreBake, false);

                if (outcome)
                {
                    thumbnailConfig.m_modelAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                        AtomToolsFramework::GetSettingsValue<AZStd::string>(ModelAssetPathSetting, DefaultModelPath)));
                    thumbnailConfig.m_materialAsset = outcome.TakeValue();
                    thumbnailConfig.m_lightingAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                        AtomToolsFramework::GetSettingsValue<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath)));
                }

                return thumbnailConfig;
            }

            const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
            if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type() && AZ::StringFunc::EndsWith(path.c_str(), ".lightingpreset.azasset"))
            {
                static constexpr const char* ModelAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingPresetAssetType/ModelAssetPath";
                static constexpr const char* MaterialAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingPresetAssetType/MaterialAssetPath";

                ThumbnailConfig thumbnailConfig;
                thumbnailConfig.m_modelAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(ModelAssetPathSetting, DefaultModelPath)));
                thumbnailConfig.m_materialAsset.Create(
                    SharedPreviewUtils::GetAssetIdForProductPath(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                        MaterialAssetPathSetting, "materials/reflectionprobe/reflectionprobevisualization.azmaterial")));
                thumbnailConfig.m_lightingAsset.Create(assetInfo.m_assetId);
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type() && AZ::StringFunc::EndsWith(path.c_str(), ".modelpreset.azasset"))
            {
                static constexpr const char* MaterialAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelPresetAssetType/MaterialAssetPath";
                static constexpr const char* LightingAssetPathSetting =
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelPresetAssetType/LightingAssetPath";

                // Model preset assets are relatively small JSON files containing a reference to a model asset and possibly other
                // parameters. The preset must be loaded to get the model asset ID. Then the preview will be rendered like any other model.
                AZ::Data::Asset<AZ::RPI::AnyAsset> asset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::AnyAsset>(assetInfo.m_assetId);
                const AZ::Render::ModelPreset& preset = *AZ::RPI::GetDataFromAnyAsset<AZ::Render::ModelPreset>(asset);

                ThumbnailConfig thumbnailConfig;
                thumbnailConfig.m_modelAsset = preset.m_modelAsset;
                thumbnailConfig.m_materialAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(MaterialAssetPathSetting, DefaultMaterialPath)));
                thumbnailConfig.m_lightingAsset.Create(SharedPreviewUtils::GetAssetIdForProductPath(
                    AtomToolsFramework::GetSettingsValue<AZStd::string>(LightingAssetPathSetting, DefaultLightingPresetPath)));
                return thumbnailConfig;
            }

            return ThumbnailConfig();
        }

        void SharedThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
        {
            if (auto previewRenderer = AZ::Interface<AtomToolsFramework::PreviewRendererInterface>::Get())
            {
                const auto& thumbnailConfig = GetThumbnailConfig(thumbnailKey);
                if (thumbnailConfig.IsValid())
                {
                    previewRenderer->AddCaptureRequest(
                        { thumbnailSize,
                          AZStd::make_shared<SharedPreviewContent>(
                              previewRenderer->GetScene(), previewRenderer->GetView(), previewRenderer->GetEntityContextId(),
                              thumbnailConfig.m_modelAsset, thumbnailConfig.m_materialAsset, thumbnailConfig.m_lightingAsset,
                              Render::MaterialPropertyOverrideMap()),
                          [thumbnailKey]()
                          {
                              // Instead of sending the notification that the thumbnail render failed, we will allow it to succeed and
                              // substitute it with a blank image. This will prevent the thumbnail system from getting stuck indefinitely
                              // with a white file icon and allow the thumbnail to reload if the asset changes in the future. The thumbnail
                              // system should be updated to support state management and recovery automatically.
                              QPixmap pixmap(1, 1);
                              pixmap.fill(Qt::black);
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey,
                                  &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                                  pixmap);
                          },
                          [thumbnailKey](const QPixmap& pixmap)
                          {
                              AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                  thumbnailKey,
                                  &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                                  pixmap);
                          } });
                }
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

        bool SharedThumbnailRenderer::ThumbnailConfig::IsValid() const
        {
            return m_modelAsset.GetId().IsValid() || m_materialAsset.GetId().IsValid() || m_lightingAsset.GetId().IsValid();
        }
    } // namespace LyIntegration
} // namespace AZ

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
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <SharedPreview/SharedPreviewContent.h>
#include <SharedPreview/SharedPreviewUtils.h>
#include <SharedPreview/SharedThumbnailRenderer.h>

namespace AZ
{
    namespace LyIntegration
    {
        template<typename T>
        void LoadPreviewAsset(AZ::Data::Asset<T>& asset, const AZ::Data::AssetId& assetId)
        {
            if (!asset.IsReady() && !asset.IsLoading() && assetId.IsValid())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                if (assetInfo.m_assetId.IsValid())
                {
                    asset.Create(assetId, true);
                }
            }
        }

        template<typename T>
        void LoadPreviewAsset(AZ::Data::Asset<T>& asset, const AZ::Data::AssetId& assetId, const AZStd::string& assetIdOverrideSettingKey)
        {
            LoadPreviewAsset(asset, AtomToolsFramework::GetSettingsObject<AZ::Data::AssetId>(assetIdOverrideSettingKey, assetId));
        }

        SharedThumbnailRenderer::SharedThumbnailRenderer()
        {
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

        bool SharedThumbnailRenderer::ThumbnailConfig::IsValid() const
        {
            return m_modelAsset.GetId().IsValid() || m_materialAsset.GetId().IsValid() || m_lightingAsset.GetId().IsValid();
        }

        SharedThumbnailRenderer::ThumbnailConfig SharedThumbnailRenderer::GetThumbnailConfig(
            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey)
        {
            const auto assetInfo = SharedPreviewUtils::GetSupportedAssetInfo(thumbnailKey);
            if (assetInfo.m_assetType == RPI::ModelAsset::RTTI_Type())
            {
                ThumbnailConfig thumbnailConfig;
                LoadPreviewAsset(thumbnailConfig.m_modelAsset, assetInfo.m_assetId);
                LoadPreviewAsset(
                    thumbnailConfig.m_materialAsset,
                    m_defaultMaterialAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/MaterialAssetId");
                LoadPreviewAsset(
                    thumbnailConfig.m_lightingAsset,
                    m_defaultLightingPresetAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelAssetType/LightingAssetId");
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::MaterialAsset::RTTI_Type())
            {
                ThumbnailConfig thumbnailConfig;
                LoadPreviewAsset(thumbnailConfig.m_materialAsset, assetInfo.m_assetId);
                LoadPreviewAsset(
                    thumbnailConfig.m_modelAsset,
                    m_defaultModelAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/ModelAssetId");
                LoadPreviewAsset(
                    thumbnailConfig.m_lightingAsset,
                    m_defaultLightingPresetAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/MaterialAssetType/LightingAssetId");
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::MaterialTypeAsset::RTTI_Type())
            {
                // Material type assets are not renderable so we generate a material asset from the material type.
                ThumbnailConfig thumbnailConfig;
                AZ::RPI::MaterialSourceData materialSourceData;
                materialSourceData.m_materialType = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                auto outcome = materialSourceData.CreateMaterialAsset(AZ::Uuid::CreateRandom(), "", false);

                if (outcome)
                {
                    thumbnailConfig.m_materialAsset = outcome.TakeValue();
                    LoadPreviewAsset(
                        thumbnailConfig.m_modelAsset,
                        m_defaultModelAsset.GetId(),
                        "/O3DE/Atom/CommonFeature/SharedPreview/MaterialTypeAssetType/ModelAssetId");
                    LoadPreviewAsset(
                        thumbnailConfig.m_lightingAsset,
                        m_defaultLightingPresetAsset.GetId(),
                        "/O3DE/Atom/CommonFeature/SharedPreview/MaterialTypeAssetType/LightingAssetId");
                }

                return thumbnailConfig;
            }

            const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
            if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type() && path.ends_with(AZ::Render::LightingPreset::Extension))
            {
                ThumbnailConfig thumbnailConfig;
                LoadPreviewAsset(thumbnailConfig.m_lightingAsset, assetInfo.m_assetId);
                LoadPreviewAsset(
                    thumbnailConfig.m_modelAsset,
                    m_defaultModelAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingPresetAssetType/ModelAssetId");
                LoadPreviewAsset(
                    thumbnailConfig.m_materialAsset,
                    m_reflectionMaterialAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/LightingPresetAssetType/MaterialAssetId");
                return thumbnailConfig;
            }

            if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type() && path.ends_with(AZ::Render::ModelPreset::Extension))
            {
                // Model preset assets are relatively small JSON files containing a reference to a model asset and possibly other
                // parameters. The preset must be loaded to get the model asset ID. Then the preview will be rendered like any other model.
                AZ::Data::Asset<AZ::RPI::AnyAsset> asset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::AnyAsset>(assetInfo.m_assetId);
                const AZ::Render::ModelPreset& preset = *AZ::RPI::GetDataFromAnyAsset<AZ::Render::ModelPreset>(asset);

                ThumbnailConfig thumbnailConfig;
                thumbnailConfig.m_modelAsset = preset.m_modelAsset;
                LoadPreviewAsset(
                    thumbnailConfig.m_materialAsset,
                    m_defaultMaterialAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelPresetAssetType/MaterialAssetId");
                LoadPreviewAsset(
                    thumbnailConfig.m_lightingAsset,
                    m_defaultLightingPresetAsset.GetId(),
                    "/O3DE/Atom/CommonFeature/SharedPreview/ModelPresetAssetType/LightingAssetId");
                return thumbnailConfig;
            }

            return ThumbnailConfig();
        }

        void SharedThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
        {
            auto previewRenderer = AZ::Interface<AtomToolsFramework::PreviewRendererInterface>::Get();
            if (!previewRenderer)
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                return;
            }

            if (!m_defaultModelAsset.IsReady() || !m_defaultLightingPresetAsset.IsReady() || !m_reflectionMaterialAsset.IsReady())
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                return;
            }

            const auto& thumbnailConfig = GetThumbnailConfig(thumbnailKey);
            if (!thumbnailConfig.IsValid())
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                return;
            }

            previewRenderer->AddCaptureRequest(
                { thumbnailSize,
                    AZStd::make_shared<SharedPreviewContent>(
                        previewRenderer->GetScene(), previewRenderer->GetView(), previewRenderer->GetEntityContextId(),
                        thumbnailConfig.m_modelAsset, thumbnailConfig.m_materialAsset, thumbnailConfig.m_lightingAsset,
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

        bool SharedThumbnailRenderer::Installed() const
        {
            return true;
        }

        void SharedThumbnailRenderer::OnSystemTick()
        {
            // models/sphere.azmodel
            LoadPreviewAsset(m_defaultModelAsset, AZ::Data::AssetId("{6DE0E9A8-A1C7-5D0F-9407-4E627C1F223C}", 284780167));
            // lightingpresets/thumbnail.lightingpreset.azasset
            LoadPreviewAsset(m_defaultLightingPresetAsset, AZ::Data::AssetId("{4F3761EF-E279-5FDD-98C3-EF90F924FBAC}", 0));
            // materials/reflectionprobe/reflectionprobevisualization.azmaterial
            LoadPreviewAsset(m_reflectionMaterialAsset, AZ::Data::AssetId("{4322FBCB-8916-5572-9CDA-18582E22D238}", 0));

            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
        }
    } // namespace LyIntegration
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomLyIntegration/CommonFeatures/Thumbnails/PreviewerFeatureProcessorProviderBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Thumbnail.h>

namespace AzFramework
{
    class Scene;
}

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QPixmap>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            class CommonPreviewRendererState;

            //! Provides custom rendering of material and model thumbnails
            class CommonPreviewRenderer
                : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler
                , public SystemTickBus::Handler
                , public PreviewerFeatureProcessorProviderBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonPreviewRenderer, AZ::SystemAllocator, 0)

                CommonPreviewRenderer();
                ~CommonPreviewRenderer();

                enum class State : AZ::s8
                {
                    None,
                    IdleState,
                    LoadState,
                    CaptureState
                };

                void SetState(State state);
                State GetState() const;

                void SelectThumbnail();
                void CancelThumbnail();
                void CompleteThumbnail();

                void LoadAssets();
                void UpdateLoadAssets();
                void CancelLoadAssets();

                void UpdateScene();
                void UpdateModel();
                void UpdateLighting();
                void UpdateCamera();

                RPI::AttachmentReadback::CallbackFunction GetCaptureCallback();
                bool StartCapture();
                void EndCapture();

            private:
                //! ThumbnailerRendererRequestsBus::Handler interface overrides...
                void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;
                bool Installed() const override;

                //! SystemTickBus::Handler interface overrides...
                void OnSystemTick() override;

                //! Render::PreviewerFeatureProcessorProviderBus::Handler interface overrides...
                void GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const override;

                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.001f;
                static constexpr float FarDist = 100.0f;
                static constexpr float FieldOfView = Constants::HalfPi;
                static constexpr float CameraRotationAngle = Constants::QuarterPi / 2.0f;

                RPI::ScenePtr m_scene;
                AZStd::string m_sceneName = "Material Thumbnail Scene";
                AZStd::string m_pipelineName = "Material Thumbnail Pipeline";
                AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
                RPI::RenderPipelinePtr m_renderPipeline;
                RPI::ViewPtr m_view;
                AZStd::vector<AZStd::string> m_passHierarchy;
                AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

                //! Incoming thumbnail requests are appended to this queue and processed one at a time in OnTick function.
                struct ThumbnailInfo
                {
                    AzToolsFramework::Thumbnailer::SharedThumbnailKey m_key;
                    int m_size = 512;
                };
                AZStd::queue<ThumbnailInfo> m_thumbnailInfoQueue;
                ThumbnailInfo m_currentThubnailInfo;

                AZStd::unordered_map<State, AZStd::shared_ptr<CommonPreviewRendererState>> m_steps;
                State m_currentState = CommonPreviewRenderer::State::None;

                static constexpr const char* DefaultLightingPresetPath = "lightingpresets/thumbnail.lightingpreset.azasset";
                const Data::AssetId DefaultLightingPresetAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultLightingPresetPath);
                Data::Asset<RPI::AnyAsset> m_defaultLightingPresetAsset;
                Data::Asset<RPI::AnyAsset> m_lightingPresetAsset;

                //! Model asset about to be rendered
                static constexpr const char* DefaultModelPath = "models/sphere.azmodel";
                const Data::AssetId DefaultModelAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultModelPath);
                Data::Asset<RPI::ModelAsset> m_defaultModelAsset;
                Data::Asset<RPI::ModelAsset> m_modelAsset;

                //! Material asset about to be rendered
                static constexpr const char* DefaultMaterialPath = "materials/basic_grey.azmaterial";
                const Data::AssetId DefaultMaterialAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultMaterialPath);
                Data::Asset<RPI::MaterialAsset> m_defaultMaterialAsset;
                Data::Asset<RPI::MaterialAsset> m_materialAsset;

                Entity* m_modelEntity = nullptr;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

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

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Feature/Utils/LightingPreset.h>

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
         //! Provides custom rendering of material thumbnails
        class MaterialThumbnailRenderer
            : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler
            , public SystemTickBus::Handler
            , public TickBus::Handler
            , public AZ::Data::AssetBus::Handler
            , public Render::FrameCaptureNotificationBus::Handler
            , public Render::MaterialComponentNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MaterialThumbnailRenderer, AZ::SystemAllocator, 0)

            MaterialThumbnailRenderer();
            ~MaterialThumbnailRenderer();

            bool Installed() const override;

            //! ThumbnailerRendererRequestsBus::Handler interface overrides...
            void RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize) override;

            //! SystemTickBus::Handler interface overrides...
            void OnSystemTick() override;

            //! AZ::TickBus::Handler interface overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetCanceled(AZ::Data::AssetId assetId) override;

            //! Render::FrameCaptureNotificationBus::Handler overrides...
            void OnCaptureFinished(Render::FrameCaptureResult result, const AZStd::string& info) override;

        private:            
            // MaterialComponentNotificationBus::Handler overrides...
            void OnMaterialsUpdated(const Render::MaterialAssignmentMap& materials) override;

            void Init();

            RPI::ScenePtr m_scene;
            AZStd::string m_sceneName = "Material Thumbnail Scene";
            AZStd::string m_pipelineName = "Material Thumbnail Pipeline";
            AzFramework::Scene* m_frameworkScene = nullptr;
            RPI::RenderPipelinePtr m_renderPipeline;
            AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;
            AZStd::vector<AZStd::string> m_passHierarchy;

            AZ::Data::Asset<AZ::RPI::AnyAsset> m_lightingPresetAsset;

            RPI::ViewPtr m_view = nullptr;
            Entity* m_modelEntity = nullptr;
            Transform m_transform;

            //! Ready to process next request, this value is accessed from different threads.
            AZStd::atomic<bool> m_shouldPullNextAsset;
            //! Is renderer initialized. Initialization is only performed once the first thumbnail request is submitted.
            bool m_initialized = false;
            //! It takes an extra frame to load a mesh and apply material, this variable is set to true once we are ready to render pipeline to texture.
            bool m_readyToCapture = false;

            //! Incoming thumbnail requests are appended to this queue and processed one at a time in OnTick function.
            AZStd::queue<Data::AssetId> m_assetIdQueue;
            //! Current material asset being rendered.
            Data::Asset<RPI::MaterialAsset> m_materialAssetToRender;

            double m_simulateTime = 0.0f;
            float m_deltaTime = 0.0f;
        };
    } // namespace LyIntegration
} // namespace AZ

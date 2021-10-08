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
#include <AtomLyIntegration/CommonFeatures/Thumbnails/PreviewerFeatureProcessorProviderBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <Thumbnails/Rendering/CommonPreviewContent.h>
#include <Thumbnails/Rendering/CommonPreviewRendererState.h>

namespace AzFramework
{
    class Scene;
}

class QImage;

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! Provides custom rendering of material and model thumbnails
            class CommonPreviewRenderer final
                : public PreviewerFeatureProcessorProviderBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonPreviewRenderer, AZ::SystemAllocator, 0);

                CommonPreviewRenderer();
                ~CommonPreviewRenderer();

                struct CaptureRequest final
                {
                    int m_size = 512;
                    AZStd::shared_ptr<CommonPreviewContent> m_content;
                    AZStd::function<void()> m_captureFailedCallback;
                    AZStd::function<void(const QImage&)> m_captureCompleteCallback;
                };

                RPI::ScenePtr GetScene() const;
                RPI::ViewPtr GetView() const;
                AZ::Uuid GetEntityContextId() const;

                void AddCaptureRequest(const CaptureRequest& captureRequest);

                enum class State : AZ::s8
                {
                    None,
                    IdleState,
                    LoadState,
                    CaptureState
                };

                void SetState(State state);
                State GetState() const;

                void SelectCaptureRequest();
                void CancelCaptureRequest();
                void CompleteCaptureRequest();

                void LoadAssets();
                void UpdateLoadAssets();
                void CancelLoadAssets();

                void UpdateScene();

                bool StartCapture();
                void EndCapture();

            private:
                //! Render::PreviewerFeatureProcessorProviderBus::Handler interface overrides...
                void GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const override;

                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.001f;
                static constexpr float FarDist = 100.0f;
                static constexpr float FieldOfView = Constants::HalfPi;

                RPI::ScenePtr m_scene;
                AZStd::string m_sceneName = "Material Thumbnail Scene";
                AZStd::string m_pipelineName = "Material Thumbnail Pipeline";
                AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
                RPI::RenderPipelinePtr m_renderPipeline;
                RPI::ViewPtr m_view;
                AZStd::vector<AZStd::string> m_passHierarchy;
                AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

                //! Incoming requests are appended to this queue and processed one at a time in OnTick function.
                AZStd::queue<CaptureRequest> m_captureRequestQueue;
                CaptureRequest m_currentCaptureRequest;

                AZStd::unordered_map<State, AZStd::shared_ptr<CommonPreviewRendererState>> m_states;
                State m_currentState = CommonPreviewRenderer::State::None;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

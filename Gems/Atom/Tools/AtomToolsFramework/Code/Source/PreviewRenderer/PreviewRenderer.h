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
#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererInterface.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewerFeatureProcessorProviderBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <PreviewRenderer/PreviewRendererState.h>

namespace AtomToolsFramework
{
    //! Processes requests for setting up content that gets rendered to a texture and captured to an image
    class PreviewRenderer final
        : public PreviewRendererInterface
        , public PreviewerFeatureProcessorProviderBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PreviewRenderer, AZ::SystemAllocator, 0);
        AZ_RTTI(PreviewRenderer, "{60FCB7AB-2A94-417A-8C5E-5B588D17F5D1}", PreviewRendererInterface);

        PreviewRenderer(const AZStd::string& sceneName, const AZStd::string& pipelineName);
        ~PreviewRenderer() override;

        void AddCaptureRequest(const PreviewRendererCaptureRequest& captureRequest) override;

        AZ::RPI::ScenePtr GetScene() const override;
        AZ::RPI::ViewPtr GetView() const override;
        AZ::Uuid GetEntityContextId() const override;

        void ProcessCaptureRequests();
        void CancelCaptureRequest();
        void CompleteCaptureRequest();

        void LoadContent();
        void UpdateLoadContent();
        void CancelLoadContent();

        void PoseContent();

        bool StartCapture();
        void EndCapture();

    private:
        //! AZ::Render::PreviewerFeatureProcessorProviderBus::Handler interface overrides...
        void GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const override;

        static constexpr float AspectRatio = 1.0f;
        static constexpr float NearDist = 0.001f;
        static constexpr float FarDist = 100.0f;
        static constexpr float FieldOfView = AZ::Constants::HalfPi;

        AZ::RPI::ScenePtr m_scene;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::RPI::ViewPtr m_view;
        AZStd::vector<AZStd::string> m_passHierarchy;
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        //! Incoming requests are appended to this queue and processed one at a time in OnTick function.
        AZStd::queue<PreviewRendererCaptureRequest> m_captureRequestQueue;
        PreviewRendererCaptureRequest m_currentCaptureRequest;

        AZStd::unique_ptr<PreviewRendererState> m_state;
    };
} // namespace AtomToolsFramework

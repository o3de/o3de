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
#include <AtomToolsFramework/PreviewRenderer/PreviewRendererState.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewerFeatureProcessorProviderBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>

namespace AzFramework
{
    class Scene;
}

class QPixmap;

namespace AtomToolsFramework
{
    //! Processes requests for setting up content that gets rendered to a texture and captured to an image
    class PreviewRenderer final : public PreviewerFeatureProcessorProviderBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PreviewRenderer, AZ::SystemAllocator, 0);

        PreviewRenderer(const AZStd::string& sceneName, const AZStd::string& pipelineName);
        ~PreviewRenderer();

        struct CaptureRequest final
        {
            int m_size = 512;
            AZStd::shared_ptr<PreviewContent> m_content;
            AZStd::function<void()> m_captureFailedCallback;
            AZStd::function<void(const QPixmap&)> m_captureCompleteCallback;
        };

        void AddCaptureRequest(const CaptureRequest& captureRequest);

        AZ::RPI::ScenePtr GetScene() const;
        AZ::RPI::ViewPtr GetView() const;
        AZ::Uuid GetEntityContextId() const;

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
        AZStd::queue<CaptureRequest> m_captureRequestQueue;
        CaptureRequest m_currentCaptureRequest;

        AZStd::unique_ptr<PreviewRendererState> m_state;
    };
} // namespace AtomToolsFramework

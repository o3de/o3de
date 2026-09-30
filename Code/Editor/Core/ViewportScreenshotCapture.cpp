/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Core/ViewportScreenshotCapture.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/std/parallel/atomic.h>

#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/Utils/PngFile.h>

namespace
{
    // Ticks to wait for a capture result before aborting a hung readback.
    constexpr int ScreenshotCaptureTickTimeout = 120;
} // namespace

ViewportScreenshotCapture::ViewportScreenshotCapture(MainStatusBar* statusBar, AZStd::string outputPath, bool antialiasing)
    : m_statusBar(statusBar)
    , m_outputPath(AZStd::move(outputPath))
    , m_antialiasing(antialiasing)
{
}

ViewportScreenshotCapture::~ViewportScreenshotCapture()
{
    // No cancel API for the readback; disconnecting here stops its callback from reaching a dead object.
    AZ::TickBus::Handler::BusDisconnect();
    AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
    DestroyPipeline();
}

bool ViewportScreenshotCapture::IsCapturing() const
{
    return m_active;
}

bool ViewportScreenshotCapture::Begin(const AZ::RPI::ScenePtr& scene, const AZ::RPI::ViewPtr& sourceView, uint32_t width, uint32_t height)
{
    if ((!scene) || !sourceView || (width == 0) || (height == 0))
    {
        return false;
    }

    m_scene = scene;

    AZ::RPI::RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.m_mainViewTagName = "MainCamera";
    static AZStd::atomic<uint32_t> s_captureCount = 0;
    pipelineDesc.m_name = AZStd::string::format("ViewportScreenshot_%u", s_captureCount.fetch_add(1));
    pipelineDesc.m_rootPassTemplate = "MainPipelineRenderToTexture";
    pipelineDesc.m_allowModification = true;
    // Match the app's MSAA count so pipeline states are shared; AA for the still is SMAA.
    pipelineDesc.m_renderSettings.m_multisampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
    pipelineDesc.m_defaultAAMethod = m_antialiasing ? "SMAA" : "MSAA";
    m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);

    if (const auto renderToTexturePass = azrtti_cast<AZ::RPI::RenderToTexturePass*>(m_renderPipeline->GetRootPass().get()))
    {
        renderToTexturePass->ResizeOutput(width, height);
    }

    scene->AddRenderPipeline(m_renderPipeline);

    // Hide screen-space editor overlays (gizmos, debug text), which can't render at an offscreen aspect.
    if (const auto& auxGeomPass = m_renderPipeline->FindFirstPass(AZ::Name("AuxGeomPass")))
    {
        auxGeomPass->SetEnabled(false);
    }
    if (const auto& twoDPass = m_renderPipeline->FindFirstPass(AZ::Name("2DPass")))
    {
        twoDPass->SetEnabled(false);
    }

    // Disable TAA passes: they assert against uninitialized attachments in the one-shot pipeline.
    for (const AZ::Name& taaPassName : { AZ::Name("TaaCopyPass"), AZ::Name("TaaPass") })
    {
        if (const auto& taaPass = m_renderPipeline->FindFirstPass(taaPassName))
        {
            taaPass->SetEnabled(false);
        }
    }

    m_passHierarchy = { m_renderPipeline->GetId().GetCStr(), "CopyToSwapChain" };

    m_view = AZ::RPI::View::CreateView(AZ::Name("MainCamera"), AZ::RPI::View::UsageCamera);
    m_renderPipeline->SetDefaultView(m_view);

    // Mirror the source view's post-process settings; processor is re-queried on teardown.
    if (auto* postProcessFeatureProcessor = scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>())
    {
        postProcessFeatureProcessor->SetViewAlias(m_view, sourceView);
    }

    m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromTransform(sourceView->GetCameraTransform()));

    // Rebuild the projection for the capture's aspect ratio to avoid stretching.
    AZ::Matrix4x4 projection = sourceView->GetViewToClipMatrix();
    AZ::SetPerspectiveMatrixFOV(
        projection, AZ::GetPerspectiveMatrixFOV(projection), aznumeric_cast<float>(width) / aznumeric_cast<float>(height));
    m_view->SetViewToClipMatrix(projection);

    // Warm up a few frames so multi-frame/async effects aren't captured as black or corrupt.
    m_renderPipeline->AddToRenderTick();
    m_warmupFramesRemaining = 2;
    m_active = true;
    AZ::TickBus::Handler::BusConnect();
    return true;
}

void ViewportScreenshotCapture::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    if (m_captureRequested)
    {
        // Abort a readback that never completes so the button isn't stuck and the pipeline is freed.
        if (++m_ticksSinceRequest > ScreenshotCaptureTickTimeout)
        {
            AZ_Warning("Editor", false, "Viewport screenshot capture timed out; aborting.");
            AZ::TickBus::Handler::BusDisconnect();
            // Detach from the readback so a late completion can't report a misleading "saved" status.
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            ReportStatus(false, AZStd::string("Screenshot capture timed out"));
            m_active = false;
            DestroyPipeline();
        }
        return;
    }

    if (m_warmupFramesRemaining > 0)
    {
        --m_warmupFramesRemaining;
        return;
    }

    AZ::Render::FrameCaptureOutcome captureOutcome;
    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
        captureOutcome,
        &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment,
        m_outputPath,
        m_passHierarchy,
        AZStd::string("Output"),
        AZ::RPI::PassAttachmentReadbackOption::Output);

    if (!captureOutcome.IsSuccess())
    {
        AZ_Warning("Editor", false, "Failed to capture viewport screenshot: %s", captureOutcome.GetError().m_errorMessage.c_str());
        AZ::TickBus::Handler::BusDisconnect();
        ReportStatus(false, captureOutcome.GetError().m_errorMessage);
        m_active = false;
        DestroyPipeline();
        return;
    }

    m_captureRequested = true;
    m_ticksSinceRequest = 0;
    AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(captureOutcome.GetValue());
}

void ViewportScreenshotCapture::OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info)
{
    m_active = false;
    AZ::TickBus::Handler::BusDisconnect();
    AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
    DestroyPipeline();

    if (result == AZ::Render::FrameCaptureResult::Success)
    {
        // Strip the (possibly transparent) alpha channel so the scene is visible in viewers.
        AZ::Utils::PngFile image = AZ::Utils::PngFile::Load(m_outputPath.c_str());
        if (image)
        {
            AZ::Utils::PngFile::SaveSettings saveSettings;
            saveSettings.m_stripAlpha = true;
            image.Save(m_outputPath.c_str(), saveSettings);
        }

        AZ_Printf("Editor", "Viewport screenshot saved to %s\n", m_outputPath.c_str());
        ReportStatus(true, m_outputPath);
    }
    else
    {
        AZ_Warning("Editor", false, "Failed to save viewport screenshot: %s", info.c_str());
        ReportStatus(false, info);
    }
}

void ViewportScreenshotCapture::ReportStatus(bool success, const AZStd::string& message)
{
    // QPointer: no-op if the status bar was destroyed before the async capture finished.
    if (!m_statusBar)
    {
        return;
    }

    m_statusBar->SetStatusText(
        success ? QString("Viewport screenshot saved to %1").arg(message.c_str())
                : QString("Failed to save viewport screenshot: %1").arg(message.c_str()));
}

void ViewportScreenshotCapture::DestroyPipeline()
{
    // Re-query the processor lazily so RemoveViewAlias never goes through a dangling raw pointer.
    if (m_scene && m_view)
    {
        if (auto* postProcessFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>())
        {
            postProcessFeatureProcessor->RemoveViewAlias(m_view);
        }
    }

    if (m_renderPipeline)
    {
        m_renderPipeline->RemoveFromRenderTick();
        if (m_scene)
        {
            m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
        }
        m_renderPipeline.reset();
    }

    m_view.reset();
    m_scene.reset();
}

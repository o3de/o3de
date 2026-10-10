/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/RPI.Public/Base.h>

#include <MainStatusBar.h>

#include <QPointer>

#include <SandboxAPI.h>

// FrameCaptureRequestBus can only read back an existing on-screen pass attachment at its current
// resolution, so this renders the viewport through its own offscreen pipeline at an arbitrary size
// (with aspect-corrected projection and post-process mirroring) and saves it as a PNG.
// The readback is asynchronous, so cross-referenced objects are held via safe handles (QPointer,
// ScenePtr, ViewPtr) so a capture outliving its scene/status bar can be torn down without dangling
// pointers; destroying the pipeline happens on shutdown for captures still in flight.
class SANDBOX_API ViewportScreenshotCapture
    : public AZ::Render::FrameCaptureNotificationBus::Handler
    , private AZ::TickBus::Handler
{
public:
    ViewportScreenshotCapture(MainStatusBar* statusBar, AZStd::string outputPath, bool antialiasing);
    ~ViewportScreenshotCapture() override;

    //! True while the capture is warming up or a readback is in flight.
    bool IsCapturing() const;

    //! Create the offscreen pipeline and begin capturing the given view at width x height.
    //! @param scene Held as a ScenePtr; its post-process processor is re-queried lazily so teardown
    //!              won't dereference a dangling pointer if the scene dies mid-flight.
    bool Begin(const AZ::RPI::ScenePtr& scene, const AZ::RPI::ViewPtr& sourceView, uint32_t width, uint32_t height);

private:
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

    //! Reports the outcome to the status bar, if it is still alive.
    void ReportStatus(bool success, const AZStd::string& message);

    //! Releases the offscreen pipeline and the aliased view. Safe to call more than once.
    void DestroyPipeline();

    // QPointer: auto-nulls if the status bar dies before the async capture finishes.
    QPointer<MainStatusBar> m_statusBar;
    AZStd::string m_outputPath;
    AZStd::vector<AZStd::string> m_passHierarchy;
    AZ::RPI::ScenePtr m_scene;
    AZ::RPI::RenderPipelinePtr m_renderPipeline;
    AZ::RPI::ViewPtr m_view;
    bool m_active = false;
    bool m_captureRequested = false;
    int m_warmupFramesRemaining = 0;
    int m_ticksSinceRequest = 0;
    bool m_antialiasing = true;
};

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <PreviewRenderer/PreviewRendererState.h>

namespace AtomToolsFramework
{
    //! PreviewRendererCaptureState renders a preview to an image
    class PreviewRendererCaptureState final
        : public PreviewRendererState
        , public AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        PreviewRendererCaptureState(PreviewRenderer* renderer);
        ~PreviewRendererCaptureState();
        void Update() override;

    private:
        //! AZ::Render::FrameCaptureNotificationBus::Handler overrides...
        void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

        //! Track the amount of time since the capture request was initiated
        AZStd::chrono::steady_clock::time_point m_startTime = AZStd::chrono::steady_clock::now();
        AZStd::chrono::steady_clock::time_point m_captureTime = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(5);
        AZStd::chrono::steady_clock::time_point m_abortTime = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(5000);
        bool m_captureComplete = false;
    };
} // namespace AtomToolsFramework

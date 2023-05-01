/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererCaptureState.h>

namespace AtomToolsFramework
{
    PreviewRendererCaptureState::PreviewRendererCaptureState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
        m_renderer->PoseContent();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    PreviewRendererCaptureState::~PreviewRendererCaptureState()
    {
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
        m_renderer->EndCapture();
    }

    void PreviewRendererCaptureState::OnSystemTick()
    {
        if (m_ticksToCapture-- <= 0)
        {
            AZ::Render::FrameCaptureId frameCaptureId = m_renderer->StartCapture();
            if (frameCaptureId != AZ::Render::InvalidFrameCaptureId)
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(frameCaptureId);
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
            // if the start capture call fails the capture will be retried next tick.
        }
    }

    void PreviewRendererCaptureState::OnFrameCaptureFinished(
        [[maybe_unused]] AZ::Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
    {
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
        m_renderer->CompleteCaptureRequest();
    }
} // namespace AtomToolsFramework


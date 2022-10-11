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
            m_frameCaptureId = m_renderer->StartCapture();
            if (m_frameCaptureId != AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId)
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
            // if the start capture call fails the capture will be retried next tick.
        }
    }

    void PreviewRendererCaptureState::OnCaptureFinished( uint32_t frameCaptureId,
        [[maybe_unused]] AZ::Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
    {
        if (frameCaptureId == m_frameCaptureId) // validate this event is for the frame capture request this state made
        {
            m_renderer->CompleteCaptureRequest();
            m_frameCaptureId = AZ::Render::FrameCaptureRequests::s_InvalidFrameCaptureId;
        }
    }
} // namespace AtomToolsFramework


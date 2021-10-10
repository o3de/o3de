/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererCaptureState.h>

namespace AtomToolsFramework
{
    PreviewRendererCaptureState::PreviewRendererCaptureState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
    }

    void PreviewRendererCaptureState::Start()
    {
        m_ticksToCapture = 1;
        m_renderer->PoseContent();
        AZ::TickBus::Handler::BusConnect();
    }

    void PreviewRendererCaptureState::Stop()
    {
        m_renderer->EndCapture();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
    }

    void PreviewRendererCaptureState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_ticksToCapture-- <= 0)
        {
            // Reset the capture flag if the capture request was successful. Otherwise try capture it again next tick.
            if (m_renderer->StartCapture())
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }

    void PreviewRendererCaptureState::OnCaptureFinished(
        [[maybe_unused]] AZ::Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
    {
        m_renderer->CompleteCaptureRequest();
    }
} // namespace AtomToolsFramework

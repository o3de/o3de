/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererIdleState.h>

namespace AtomToolsFramework
{
    PreviewRendererIdleState::PreviewRendererIdleState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
    }

    void PreviewRendererIdleState::Start()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void PreviewRendererIdleState::Stop()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void PreviewRendererIdleState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_renderer->SelectCaptureRequest();
    }
} // namespace AtomToolsFramework

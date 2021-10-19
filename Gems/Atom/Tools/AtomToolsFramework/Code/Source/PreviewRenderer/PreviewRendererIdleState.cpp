/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererIdleState.h>

namespace AtomToolsFramework
{
    PreviewRendererIdleState::PreviewRendererIdleState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
        AZ::TickBus::Handler::BusConnect();
    }

    PreviewRendererIdleState::~PreviewRendererIdleState()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void PreviewRendererIdleState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_renderer->ProcessCaptureRequests();
    }
} // namespace AtomToolsFramework

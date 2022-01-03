/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererLoadState.h>

namespace AtomToolsFramework
{
    PreviewRendererLoadState::PreviewRendererLoadState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
        m_renderer->LoadContent();
        AZ::TickBus::Handler::BusConnect();
    }

    PreviewRendererLoadState::~PreviewRendererLoadState()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void PreviewRendererLoadState::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if ((m_timeRemainingS += deltaTime) > TimeOutS)
        {
            m_renderer->CancelLoadContent();
            return;
        }

        m_renderer->UpdateLoadContent();
    }
} // namespace AtomToolsFramework

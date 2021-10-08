/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererLoadState.h>

namespace AtomToolsFramework
{
    PreviewRendererLoadState::PreviewRendererLoadState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
    }

    void PreviewRendererLoadState::Start()
    {
        m_renderer->LoadAssets();
        m_timeRemainingS = TimeOutS;
        AZ::TickBus::Handler::BusConnect();
    }

    void PreviewRendererLoadState::Stop()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void PreviewRendererLoadState::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_timeRemainingS -= deltaTime;
        if (m_timeRemainingS > 0.0f)
        {
            m_renderer->UpdateLoadAssets();
        }
        else
        {
            m_renderer->CancelLoadAssets();
        }
    }
} // namespace AtomToolsFramework

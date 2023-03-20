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
        AZ::SystemTickBus::Handler::BusConnect();
    }

    PreviewRendererLoadState::~PreviewRendererLoadState()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void PreviewRendererLoadState::OnSystemTick()
    {
        if (AZStd::chrono::steady_clock::now() > (m_startTime + AZStd::chrono::seconds(5)))
        {
            m_renderer->CancelLoadContent();
            return;
        }

        m_renderer->UpdateLoadContent();
    }
} // namespace AtomToolsFramework

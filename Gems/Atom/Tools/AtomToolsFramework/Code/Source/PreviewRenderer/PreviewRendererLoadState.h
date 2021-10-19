/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <PreviewRenderer/PreviewRendererState.h>

namespace AtomToolsFramework
{
    //! PreviewRendererLoadState pauses further rendering until all assets used for rendering a thumbnail have been loaded
    class PreviewRendererLoadState final
        : public PreviewRendererState
        , public AZ::TickBus::Handler
    {
    public:
        PreviewRendererLoadState(PreviewRenderer* renderer);
        ~PreviewRendererLoadState();

    private:
        //! AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        static constexpr float TimeOutS = 5.0f;
        float m_timeRemainingS = 0.0f;
    };
} // namespace AtomToolsFramework

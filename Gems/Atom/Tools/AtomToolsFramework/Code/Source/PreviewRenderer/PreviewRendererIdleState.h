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
    //! PreviewRendererIdleState checks whether there are any new thumbnails that need to be rendered every tick
    class PreviewRendererIdleState final
        : public PreviewRendererState
        , public AZ::TickBus::Handler
    {
    public:
        PreviewRendererIdleState(PreviewRenderer* renderer);
        ~PreviewRendererIdleState();

    private:
        //! AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };
} // namespace AtomToolsFramework

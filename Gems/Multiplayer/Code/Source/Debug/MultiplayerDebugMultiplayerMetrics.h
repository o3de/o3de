/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/Component/Entity.h>

#ifdef IMGUI_ENABLED
#   include <imgui/imgui.h>
#   include <ImGuiBus.h>
#   include <LYImGuiUtils/HistogramContainer.h>
#endif

namespace Multiplayer
{
    class MultiplayerDebugMultiplayerMetrics final
    {
    public:
        MultiplayerDebugMultiplayerMetrics() = default;
        ~MultiplayerDebugMultiplayerMetrics() = default;

#ifdef IMGUI_ENABLED
        void OnImGuiUpdate();
#endif
    };
}

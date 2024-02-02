/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef IMGUI_ENABLED
#include "ImGuiManager.h"
#include "ImGuiBus.h"
#include "LYImGuiUtils/HistogramContainer.h"
#include <AzCore/Component/TickBus.h>

namespace ImGui
{
    // basic class to show the state of the AzFramework input system.
    // Just dumps all the known devices, whether they're connected or not, and if they have input channels, what the state of those
    // channels are.
    class ImGuiInputMonitor
    {
    public:
        ImGuiInputMonitor();
        virtual ~ImGuiInputMonitor() = default;

        // Called from owner
        void Initialize();
        void Shutdown();

        // Draw the ImGui Menu
        void ImGuiUpdate();

        // Toggle the menu on and off
        void ToggleEnabled() { m_enabled = !m_enabled; }

    private:
        // flag for if the entire Menu enabled / visible
        bool m_enabled;

        void ImGuiUpdate_DrawMenu();
    };
}
#endif // IMGUI_ENABLED

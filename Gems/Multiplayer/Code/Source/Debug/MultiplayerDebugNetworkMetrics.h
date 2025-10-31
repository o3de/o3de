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
    class MultiplayerDebugNetworkMetrics final
    {
    public:
#ifdef IMGUI_ENABLED
        void OnImGuiUpdate();

    private:
        struct NetworkMetricDisplay
        {
            int64_t m_lastValue = 0;
            ImGui::LYImGuiUtils::HistogramContainer m_histogram;
        };
        AZStd::unordered_map<AZ::Name, NetworkMetricDisplay> m_sendHistograms;
        AZStd::unordered_map<AZ::Name, NetworkMetricDisplay> m_recvHistograms;
#endif
    };
}

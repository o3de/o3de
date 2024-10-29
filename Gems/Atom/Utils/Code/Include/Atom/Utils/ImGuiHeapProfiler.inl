/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <imgui/imgui.h>
#include <Atom/RHI.Reflect/AllocatorManager.h>

namespace AZ::Render
{
    inline void ImGuiHeapProfiler::Draw(bool& draw)
    {
        using namespace AZ;
        static constexpr size_t MB = 1u << 20;

        ImGui::SetNextWindowSize(ImVec2(200.f, 200.f), ImGuiCond_FirstUseEver);
        // Draw the hierarchical view
        // It will assign m_seletedPass if there is a pass matches m_seletedPassPath
        ImGui::SetNextWindowPos(ImVec2(300, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(950, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Heap Profiler", &draw, ImGuiWindowFlags_None))
        {
            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
            constexpr int NumColumns = 4;
            if (ImGui::BeginTable("table", NumColumns, flags))
            {
                ImGui::TableSetupColumn("Allocator Name", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Current Memory Requested (MB)");
                ImGui::TableSetupColumn("Peak Memory Requested (MB)");
                ImGui::TableSetupColumn("Requested Allocations");
                ImGui::TableHeadersRow();

                size_t requestedBytes;
                size_t requestedAllocs;
                size_t requestedBytesPeak;
                AZStd::vector<RHI::AllocatorManager::AllocatorStats> stats;
                RHI::AllocatorManager::Instance().GetAllocatorStats(requestedBytes, requestedAllocs, requestedBytesPeak, &stats);
                for (const auto& stat : stats)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(stat.m_name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", static_cast<float>(stat.m_requestedBytes) / MB);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", static_cast<float>(stat.m_requestedBytesPeak) / MB);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", static_cast<int >(stat.m_requestedAllocs));
                }

                ImGui::EndTable();
            }

            ImVec2 buttonSize(170.f, 0.f);
            float widthNeeded = buttonSize.x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);
            if (ImGui::Button("Reset Peak Memory", buttonSize))
            {
                RHI::AllocatorManager::Instance().ResetPeakBytes();
            }
        }
        ImGui::End();       
    }
}

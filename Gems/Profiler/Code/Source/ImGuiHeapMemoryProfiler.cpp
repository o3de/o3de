/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(IMGUI_ENABLED)

#include <ImGuiHeapMemoryProfiler.h>
#include <AzCore/std/sort.h>

namespace Profiler
{
    enum HeapProfilerColumnID
    {
        HeapProfilerColumnID_Name = 0,
        HeapProfilerColumnID_ParentName,
        HeapProfilerColumnID_AllocatedMem,
        HeapProfilerColumnID_CapacityMem
    };

    void ImGuiHeapMemoryProfiler::Draw(bool& draw)
    {
        using namespace AZ;
        static constexpr size_t KB = 1u << 10;

        ImGui::SetNextWindowPos(ImVec2(300, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Heap Profiler", &draw, ImGuiWindowFlags_None))
        {
            ImGui::Text("Allocator Name Filter (inc, -exc)");
            ImGui::SameLine();
            m_filter.Draw("");            
            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable;
            constexpr int NumColumns = 4;
            if (ImGui::BeginTable("table", NumColumns, flags))
            {
                ImGui::TableSetupColumn(
                    "Allocator Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 0.f, HeapProfilerColumnID_Name);
                ImGui::TableSetupColumn("Allocated Memory (kB)", 0, 0.f, HeapProfilerColumnID_AllocatedMem);
                ImGui::TableSetupColumn("Capacity Memory (kB)", 0, 0.f, HeapProfilerColumnID_CapacityMem);
                ImGui::TableSetupColumn("Parent Name", 0, 0.f, HeapProfilerColumnID_ParentName);
                ImGui::TableHeadersRow();

                size_t requestedBytes;
                size_t capacityBytes;
                AZStd::vector<AZ::AllocatorManager::AllocatorStats> stats;
                AZ::AllocatorManager::Instance().GetAllocatorStats(requestedBytes, capacityBytes, &stats);

                if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                {
                    AZStd::sort(
                        stats.begin(),
                        stats.end(),
                        [&](const AZ::AllocatorManager::AllocatorStats& lhs, const AZ::AllocatorManager::AllocatorStats& rhs)
                        {
                            const AZ::AllocatorManager::AllocatorStats* left = &lhs;
                            const AZ::AllocatorManager::AllocatorStats* right = &rhs;
                            if (sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending)
                            {
                                AZStd::swap(left, right);
                            }

                            switch (sorts_specs->Specs->ColumnUserID)
                            {
                            case HeapProfilerColumnID_Name:
                                return left->m_name < right->m_name;
                            case HeapProfilerColumnID_AllocatedMem:
                                return left->m_allocatedBytes < right->m_allocatedBytes;
                            case HeapProfilerColumnID_CapacityMem:
                                return left->m_capacityBytes < right->m_capacityBytes;
                            case HeapProfilerColumnID_ParentName:
                                return left->m_parentName < right->m_parentName;
                            default:
                                return false;
                            }
                        });
                        sorts_specs->SpecsDirty = false;
                }

                for (const auto& stat : stats)
                {
                    if (m_filter.PassFilter(stat.m_name.c_str()))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(stat.m_name.c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text("%.1f", static_cast<float>(stat.m_allocatedBytes) / KB);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.1f", static_cast<float>(stat.m_capacityBytes) / KB);
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(stat.m_parentName.c_str());
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();       
    }
}
#endif

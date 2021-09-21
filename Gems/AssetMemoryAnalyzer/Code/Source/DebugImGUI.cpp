/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetMemoryAnalyzer.h"
#include "AssetMemoryAnalyzerSystemComponent.h"
#include "DebugImGUI.h"
#include "FormatUtils.h"

#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/std/sort.h>
#include <imgui/imgui.h>
#include <ImGuiBus.h>

namespace AssetMemoryAnalyzer
{
    namespace
    {
        template<Data::AllocationCategories Category>
        struct SortFunctions
        {
            static bool SortChildAssetsByAllocatedMemory(const Data::AssetInfo* lhs, const Data::AssetInfo* rhs)
            {
                return lhs->m_totalSummary[(int)Category].m_allocatedMemory > rhs->m_totalSummary[(int)Category].m_allocatedMemory;
            }

            static bool SortAllocationPointsByAllocatedMemory(const Data::AllocationPoint* lhs, const Data::AllocationPoint* rhs)
            {
                return (lhs->m_codePoint->m_category == rhs->m_codePoint->m_category) ? (lhs->m_totalAllocatedMemory > rhs->m_totalAllocatedMemory) : (lhs->m_codePoint->m_category == Category);
            }

            static bool SortChildAssetsByAllocationCount(const Data::AssetInfo* lhs, const Data::AssetInfo* rhs)
            {
                return lhs->m_totalSummary[(int)Category].m_allocationCount > rhs->m_totalSummary[(int)Category].m_allocationCount;
            }

            static bool SortAllocationPointsByAllocationCount(const Data::AllocationPoint* lhs, const Data::AllocationPoint* rhs)
            {
                return (lhs->m_codePoint->m_category == rhs->m_codePoint->m_category) ? (lhs->m_allocations.size() > rhs->m_allocations.size()) : (lhs->m_codePoint->m_category == Category);
            }

        };
    }

    static const ImVec4 COLUMN_HEADER_COLOR(0.7f, 0.4f, 0.2f, 1.0f);
    static const float COLUMN_WIDTH = 128.0f;

    DebugImGUI::DebugImGUI()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    DebugImGUI::~DebugImGUI()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
    }

    void DebugImGUI::Init(AssetMemoryAnalyzerSystemComponent* owner)
    {
        m_owner = owner;
        m_childAssetSortFn = &SortFunctions<Data::AllocationCategories::HEAP>::SortChildAssetsByAllocatedMemory;
        m_allocationPointSortFn = &SortFunctions<Data::AllocationCategories::HEAP>::SortAllocationPointsByAllocatedMemory;
    }

    void DebugImGUI::OnImGuiUpdate()
    {
        using namespace Data;

        // Append to main menu at top of screen.
        if (ImGui::BeginMainMenuBar())
        {
            // Add new menu items.
            if (ImGui::BeginMenu("AssetMemoryAnalyzer"))
            {
                if (ImGui::Button(m_enabled == false ? "Open" : "Close"))
                {
                    ImGui::CloseCurrentPopup();
                    m_enabled = !m_enabled;
                }

                if (ImGui::Button("Export JSON"))
                {
                    EBUS_EVENT(AssetMemoryAnalyzerRequestBus, ExportJSONFile, nullptr);
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::Button("Export CSV (top-level only)"))
                {
                    EBUS_EVENT(AssetMemoryAnalyzerRequestBus, ExportCSVFile, nullptr);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (m_enabled)
        {
            // Draw the asset memory analysis window and its contents
            ImGui::Begin("Asset Memory Analysis", &m_enabled);

#ifndef AZ_TRACK_ASSET_SCOPES
            ImGui::TextColored(ImColor(255, 32, 32), "Asset scope tracking disabled in code. Recompile with AZ_TRACK_ASSET_SCOPES defined (see AssetTracking.h).");
#endif
            if (!m_owner->IsEnabled())
            {
                ImGui::TextColored(ImColor(255, 32, 32), "Asset memory analysis must be enabled by setting the \"assetmem_enable\" CVar to 1.");
            }

            AZStd::shared_ptr<FrameAnalysis> analysis = m_owner->GetAnalysis();

            if (analysis)
            {
                if (ImGui::Button("Heap Allocation Size"))
                {
                    m_childAssetSortFn = &SortFunctions<AllocationCategories::HEAP>::SortChildAssetsByAllocatedMemory;
                    m_allocationPointSortFn = &SortFunctions<AllocationCategories::HEAP>::SortAllocationPointsByAllocatedMemory;
                }
                ImGui::SameLine();

                if (ImGui::Button("Heap Allocation Count"))
                {
                    m_childAssetSortFn = &SortFunctions<AllocationCategories::HEAP>::SortChildAssetsByAllocationCount;
                    m_allocationPointSortFn = &SortFunctions<AllocationCategories::HEAP>::SortAllocationPointsByAllocationCount;
                }
                ImGui::SameLine();

                if (ImGui::Button("VRAM Allocation Size"))
                {
                    m_childAssetSortFn = &SortFunctions<AllocationCategories::VRAM>::SortChildAssetsByAllocatedMemory;
                    m_allocationPointSortFn = &SortFunctions<AllocationCategories::VRAM>::SortAllocationPointsByAllocatedMemory;
                }
                ImGui::SameLine();

                if (ImGui::Button("VRAM Allocation Count"))
                {
                    m_childAssetSortFn = &SortFunctions<AllocationCategories::VRAM>::SortChildAssetsByAllocationCount;
                    m_allocationPointSortFn = &SortFunctions<AllocationCategories::VRAM>::SortAllocationPointsByAllocationCount;
                }
                ImGui::SameLine();

                if (ImGui::Button("A -> Z"))
                {
                    m_childAssetSortFn = [](const AssetInfo* lhs, const AssetInfo* rhs) { return strcmp(lhs->m_id, rhs->m_id) < 0; };
                    m_allocationPointSortFn = [](const AllocationPoint* lhs, const AllocationPoint* rhs) {
                        int cmp = strcmp(lhs->m_codePoint->m_file, rhs->m_codePoint->m_file);
                        return (cmp < 0) || (cmp == 0 && lhs->m_codePoint->m_line < rhs->m_codePoint->m_line);
                    };
                }

                ImGui::Text("Asset/Allocation");
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - COLUMN_WIDTH * 2);
                ImGui::Text("Heap (#/kB)");
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - COLUMN_WIDTH);
                ImGui::Text("VRAM (#/kB)");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 32, 1.0));
                OutputLine("Totals", analysis->GetRootAsset().m_totalSummary[(int)AllocationCategories::HEAP], analysis->GetRootAsset().m_totalSummary[(int)AllocationCategories::VRAM]);
                ImGui::PopStyleColor();

                AZStd::function<void(const AssetInfo*, int depth)> recurse;
                recurse = [this, &recurse](const AssetInfo* asset, int depth)
                {
                    AZStd::vector<const AssetInfo*, AZ::OSStdAllocator> childAssetSorter;
                    childAssetSorter.resize(asset->m_childAssets.size());
                    AZStd::transform(asset->m_childAssets.begin(), asset->m_childAssets.end(), childAssetSorter.begin(), [](const AssetInfo& ai) { return &ai; });
                    AZStd::sort(childAssetSorter.begin(), childAssetSorter.end(), m_childAssetSortFn);

                    m_allocationPointSorter.resize(asset->m_allocationPoints.size());
                    AZStd::transform(asset->m_allocationPoints.begin(), asset->m_allocationPoints.end(), m_allocationPointSorter.begin(), [](const AllocationPoint& ap) { return &ap; });
                    AZStd::sort(m_allocationPointSorter.begin(), m_allocationPointSorter.end(), m_allocationPointSortFn);

                    if (asset->m_id)
                    {
                        float prevX = ImGui::GetCursorPosX();
                        OutputLine(nullptr, asset->m_totalSummary[(int)AllocationCategories::HEAP], asset->m_totalSummary[(int)AllocationCategories::VRAM]);
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(prevX);
                        if (ImGui::TreeNode(asset->m_id))
                        {
                            prevX = ImGui::GetCursorPosX();
                            OutputLine(nullptr, asset->m_localSummary[(int)AllocationCategories::HEAP], asset->m_localSummary[(int)AllocationCategories::VRAM]);
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(prevX);

                            if (ImGui::TreeNode("Scope allocations:"))
                            {
                                for (auto ap : m_allocationPointSorter)
                                {
                                    Summary heapSummary;
                                    Summary vramSummary;

                                    switch (ap->m_codePoint->m_category)
                                    {
                                    case AllocationCategories::HEAP:
                                        ImGui::Text("%s", FormatUtils::FormatCodePoint(*ap->m_codePoint));
                                        heapSummary.m_allocationCount = static_cast<uint32_t>(ap->m_allocations.size());
                                        heapSummary.m_allocatedMemory = ap->m_totalAllocatedMemory;
                                        break;

                                    case AllocationCategories::VRAM:
                                        ImGui::Text("%s", ap->m_codePoint->m_file);
                                        vramSummary.m_allocationCount = static_cast<uint32_t>(ap->m_allocations.size());
                                        vramSummary.m_allocatedMemory = ap->m_totalAllocatedMemory;
                                        break;
                                    }

                                    ImGui::SameLine();
                                    OutputLine(nullptr, heapSummary, vramSummary);
                                }

                                ImGui::TreePop();
                            }

                            for (auto child : childAssetSorter)
                            {
                                recurse(child, depth + 1);
                            }

                            ImGui::TreePop();
                        }
                    }
                    else
                    {
                        for (auto child : childAssetSorter)
                        {
                            recurse(child, depth + 1);
                        }
                    }
                };

                recurse(&analysis->GetRootAsset(), 0);
            }

            ImGui::End();
        }
    }

    void DebugImGUI::OutputLine(const char* text, const Data::Summary& heapSummary, const Data::Summary& vramSummary)
    {
        if (text)
        {
            ImGui::Text("%s", text);
            ImGui::SameLine();
        }

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - COLUMN_WIDTH * 2);
        OutputField(heapSummary);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - COLUMN_WIDTH);
        OutputField(vramSummary);
    }

    void DebugImGUI::OutputField(const Data::Summary& summary)
    {
        if (summary.m_allocationCount)
        {
            ImGui::Text("%u / %s", summary.m_allocationCount, FormatUtils::FormatKB(summary.m_allocatedMemory));
        }
        else
        {
            ImGui::Text("-- / --");
        }
    }
}

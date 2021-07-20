/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <ImGuiBus.h>

namespace AssetMemoryAnalyzer
{
    namespace Data
    {
        struct AllocationPoint;
        struct AssetInfo;
        struct Summary;
    }

    class AssetMemoryAnalyzerSystemComponent;

    // This class provides debug UI for the gem using ImGUI.
    class DebugImGUI 
        : public ImGui::ImGuiUpdateListenerBus::Handler
    {
    public:
        AZ_TYPE_INFO(AssetMemoryAnalyzer::DebugImGUI, "{D121DA34-EF16-46C2-AFC4-A1EE69DA0851}");
        AZ_CLASS_ALLOCATOR(DebugImGUI, AZ::OSAllocator, 0);

        DebugImGUI();
        ~DebugImGUI();

        void Init(AssetMemoryAnalyzerSystemComponent* owner);

        // ImGuiUpdateListenerBus
        void OnImGuiUpdate() override;


    private:
        void OutputLine(const char* text, const Data::Summary& heapSummary, const Data::Summary& vramSummary);
        void OutputField(const Data::Summary& summary);

        AssetMemoryAnalyzerSystemComponent* m_owner;
        bool (*m_childAssetSortFn)(const Data::AssetInfo* lhs, const Data::AssetInfo* rhs) = nullptr;
        AZStd::vector<const Data::AssetInfo*, AZ::OSStdAllocator> m_childAssetSorter;
        bool (*m_allocationPointSortFn)(const Data::AllocationPoint* lhs, const Data::AllocationPoint* rhs) = nullptr;
        AZStd::vector<const Data::AllocationPoint*, AZ::OSStdAllocator> m_allocationPointSorter;
        bool m_enabled = false;
    };
}

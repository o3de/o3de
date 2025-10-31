/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <Profiler/ImGuiTreemap.h>

namespace Profiler
{
    class ImGuiTreemapImpl : public ImGuiTreemap
    {
    public:
        ~ImGuiTreemapImpl() override = default;

        const AZ::Name& GetName() const override;
        void SetName(const char* name) override;
        void SetName(AZ::Name name) override;
        void SetUnitLabel(const char* unitLabel) override;
        void AddMask(const char* label, uint32_t mask) override;
        void SetRoots(AZStd::vector<TreemapNode>&& roots) override;
        void Render(int x, int y, int w, int h) override;
        void WeighAndComputeLayout(int w, int h) override;

    private:
        void WeighNodes();
        void AssignColors();
        void ComputeLayout(int w, int h);
        void SquarifyChildren(TreemapNode& node);

        AZ::Name m_name;

        const char* m_unitLabel;

        TreemapNode m_root;

        // Stores pointers to nodes at the same depth level. This is needed for both weight normalization
        // and back-to-front drawing for ImGui
        AZStd::vector<AZStd::vector<TreemapNode*>> m_levelSets;

        // Masks are used to include or exclude nodes from the treemap
        AZStd::unordered_map<const char*, uint32_t> m_masks;

        // The total sum of all weights for nodes occupying a given depth in the tree
        AZStd::vector<float> m_levelWeights;

        // The set of groups used to tag constituent nodes
        struct GroupConfig
        {
            bool m_active = false;
            float m_hue;
            float m_saturation;
        };
        AZStd::unordered_map<AZ::Name, GroupConfig> m_groups;

        // This set of nodes are reset each frame and used to store treemap nodes beneath the user cursor
        AZStd::vector<TreemapNode*> m_tooltipNodes;

        TreemapNode* m_selectedNode = nullptr;

        // A common pattern in the implementation is to use a stack to traverse the tree via BFS. Keeping
        // the stack around as a member variable avoids unnecessary allocations and maintains a reservation
        // that matches the maximum size the stack expands to.
        AZStd::vector<TreemapNode*> m_stack;
        int m_lastExtent[2] = { 0, 0 };
        uint32_t m_currentMask = 0xffffffff;
        bool m_dirty = true;
    };

    class ImGuiTreemapFactoryImpl : public ImGuiTreemapFactory
    {
     public:
        ImGuiTreemap& Create(AZ::Name name, const char* unitLabel) override;
        void Destroy(ImGuiTreemap* treemap) override;

     private:
        AZStd::mutex m_mutex;
        AZStd::unordered_map<AZ::Name, ImGuiTreemapImpl> m_treemaps;
    };
}

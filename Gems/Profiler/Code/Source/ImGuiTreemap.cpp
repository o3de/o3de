/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#define PROFILER_IMGUI_DLL
#include "ImGuiTreemapImpl.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/Math/Crc.h>
#include <imgui/imgui.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using AZ::Name;

#define CHECK_STACK_EMPTY()                                                                                                                \
    AZ_Assert(                                                                                                                             \
        m_stack.empty(),                                                                                                                   \
        "The treemap stack was empty at the start of a traversal. This indicates a bug in the treemap implementation, so please file a "   \
        "ticket and/or notify sig-core.")

namespace Profiler
{
    static void NameToHueSaturation(AZ::Name name, float& outHue, float& outSaturation)
    {
        uint32_t hash = AZ::Crc32{ name.GetStringView() };
        // Apply one round of LCG (constants from glibc)
        hash *= 1103515245u;
        hash += 12345u;

        // The explicit cast of the uint32 max quantity is needed because on cast, the value increases by 1.
        outHue = static_cast<float>(hash) / static_cast<float>(0xffffffff);

        outSaturation = 0.7f;
    }

    ImGuiTreemap& ImGuiTreemapFactoryImpl::Create(AZ::Name name, const char* unitLabel)
    {
        AZStd::scoped_lock lock{ m_mutex };
        AZ_Assert(!m_treemaps.contains(name), "Attempting to create treemap %s but it already exists!", name.GetCStr());
        ImGuiTreemap& treemap = m_treemaps.emplace(name).first->second;
        treemap.SetName(AZStd::move(name));
        treemap.SetUnitLabel(unitLabel);
        return treemap;
    }

    void ImGuiTreemapFactoryImpl::Destroy(ImGuiTreemap* treemap)
    {
        AZStd::scoped_lock lock{ m_mutex };
        AZ_Assert(
            m_treemaps.contains(treemap->GetName()), "Attempting to destroy treemap %s but it doesn't exist!",
            treemap->GetName().GetCStr());
        m_treemaps.erase(treemap->GetName());
    }

    const Name& ImGuiTreemapImpl::GetName() const
    {
        return m_name;
    }

    void ImGuiTreemapImpl::SetName(char const* name)
    {
        m_name = Name{ name };
    }

    void ImGuiTreemapImpl::SetName(Name name)
    {
        m_name = AZStd::move(name);
    }

    void ImGuiTreemapImpl::SetUnitLabel(const char* unitLabel)
    {
        m_unitLabel = unitLabel;
    }

    void ImGuiTreemapImpl::AddMask(const char* label, uint32_t mask)
    {
        m_masks[label] = mask;
    }

    void ImGuiTreemapImpl::SetRoots(AZStd::vector<TreemapNode>&& roots)
    {
        m_root.m_children = AZStd::move(roots);
        m_groups.clear();
        m_dirty = true;
        // Reset extent to trigger re-layout
        m_lastExtent[0] = 0;
        m_selectedNode = nullptr;
        m_currentMask = 0xffffffff;
    }

    void ImGuiTreemapImpl::Render(int x, int y, int w, int h)
    {
        const char* treemapName = m_name.IsEmpty() ? "Unnamed treemap" : m_name.GetCStr();

        if (m_root.m_children.empty())
        {
            ImGui::Begin(treemapName);
            ImGui::End();
            return;
        }

        ImVec2 offset{ static_cast<float>(x), static_cast<float>(y) };
        ImGui::SetNextWindowPos(offset, ImGuiCond_Once);
        ImVec2 extent{ static_cast<float>(w), static_cast<float>(h) };
        ImGui::SetNextWindowSize(extent, ImGuiCond_Once);

        if (ImGui::Begin(treemapName))
        {
            ImGui::Text("Total weight: %f %s", m_root.m_weight, m_unitLabel);
            if (!m_groups.empty())
            {
                ImGui::Text("Highlight Group");
            }
            for (auto& group : m_groups)
            {
                ImGui::SameLine();
                ImGui::Checkbox(group.first.GetCStr(), &group.second.m_active);
            }

            if (!m_masks.empty())
            {
                if (ImGui::RadioButton("Display All", m_currentMask == 0xffffffff))
                {
                    m_currentMask = 0xffffffff;
                    m_dirty = true;
                    m_lastExtent[0] = 0;
                    m_selectedNode = nullptr;
                }

                for (auto& mask : m_masks)
                {
                    ImGui::SameLine();
                    if (ImGui::RadioButton(mask.first, m_currentMask == mask.second))
                    {
                        m_currentMask = mask.second;
                        m_dirty = true;
                        m_lastExtent[0] = 0;
                        m_selectedNode = nullptr;
                    }
                }
            }

            if (m_selectedNode)
            {
                ImGui::Text(
                    "Selected node: %s (%f %s)", m_selectedNode->m_name.IsEmpty() ? "[unnamed]" : m_selectedNode->m_name.GetCStr(),
                    m_selectedNode->m_weight, m_unitLabel);
            }
            else
            {
                ImGui::Text("No node selected");
            }
            ImGui::Separator();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const auto [cx, cy] = ImGui::GetCursorPos();
            const auto [wx, wy] = ImGui::GetWindowPos();
            const auto [ww, wh] = ImGui::GetWindowSize();
            bool focused = ImGui::IsWindowFocused();

            // Add 20 pixel gutter to bottom
            float treemapOffset[2] = { cx + wx, cy + wy };

            WeighAndComputeLayout(static_cast<int>(ww - cx), static_cast<int>(wh - cy) - 20);

            const auto [mx, my] = ImGui::GetMousePos();

            m_tooltipNodes.clear();

            // Draw nodes starting at the top level (ignoring the root) and descend down
            for (auto it = m_levelSets.begin() + 1; it != m_levelSets.end(); ++it)
            {
                for (TreemapNode* node : *it)
                {
                    if (node->m_area < 1e-5 || (node->m_tag != 0 && (node->m_tag & m_currentMask) == 0))
                    {
                        continue;
                    }

                    ImVec2 topLeft{ static_cast<float>(node->m_offset[0]) + treemapOffset[0], static_cast<float>(node->m_offset[1]) + treemapOffset[1] };
                    ImVec2 nodeExtent{ static_cast<float>(node->m_extent[0]), static_cast<float>(node->m_extent[1]) };

                    if (node->m_children.empty())
                    {
                        // Shrink leaf nodes with an additional 2 pixel gutter
                        topLeft.x += 2;
                        topLeft.y += 2;
                        nodeExtent.x -= 4;
                        nodeExtent.y -= 4;
                    }

                    ImVec2 bottomRight{ topLeft.x + nodeExtent.x, topLeft.y + nodeExtent.y };
                    float r;
                    float g;
                    float b;
                    float saturationShift = 0.f;

                    if (focused && mx > topLeft.x && mx < bottomRight.x && my > topLeft.y && my < bottomRight.y)
                    {
                        // Mouse is hovering over this node. Add it as a node to display in the tooltip
                        saturationShift += 0.15f;
                        m_tooltipNodes.push_back(node);

                        if (focused && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && node->m_children.empty())
                        {
                            if (m_selectedNode == node)
                            {
                                m_selectedNode = nullptr;
                            }
                            else
                            {
                                m_selectedNode = node;
                            }
                        }
                    }

                    bool selected = m_selectedNode == node;

                    if (!node->m_group.IsEmpty() && m_groups[node->m_group].m_active)
                    {
                        const GroupConfig& groupConfig = m_groups[node->m_group];
                        ImGui::ColorConvertHSVtoRGB(
                            groupConfig.m_hue,
                            groupConfig.m_saturation + saturationShift,
                            (selected ? 0.9f : node->m_value) + 0.1f,
                            r, g, b);
                    }
                    else
                    {
                        ImGui::ColorConvertHSVtoRGB(
                            node->m_hue, node->m_saturation + saturationShift, selected ? 1.f : node->m_value, r, g, b);
                    }

                    drawList->AddRectFilled(topLeft, bottomRight, ImColor{ r, g, b }, 2.f, ImDrawFlags_RoundCornersAll);
                }
            }

            if (!m_tooltipNodes.empty())
            {
                ImGui::BeginTooltip();
                for (TreemapNode* node : m_tooltipNodes)
                {
                    if (!node->m_tooltip.empty())
                    {
                        ImGui::Text("%s", node->m_tooltip.c_str());
                    }
                    else if (!node->m_name.IsEmpty())
                    {
                        ImGui::Text("%s (%f %s)", node->m_name.GetCStr(), node->m_weight, m_unitLabel);
                    }
                    else
                    {
                        ImGui::Text("[unnamed] (%f %s)", node->m_weight, m_unitLabel);
                    }
                    ImGui::Indent();
                }
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }

    void ImGuiTreemapImpl::WeighAndComputeLayout(int w, int h)
    {
        WeighNodes();
        ComputeLayout(w, h);
        AssignColors();
        m_dirty = false;
    }

    void ImGuiTreemapImpl::WeighNodes()
    {
        // The goal of this function is to ensure that every node in the tree starting with m_root has the
        // following data initialized:
        // - m_parent (pointer to the parent node)
        // - m_level (depth of the node (e.g. distance from the root node)
        // - m_weight (cumulative some of weights of all children, descending to the leaves)
        //
        // In addition, this function computes the values in m_levelWeights, which is the sum of the weights
        // for all nodes at each depth level in the tree

        if (!m_dirty)
        {
            return;
        }

        // Flatten the tree via BFS into different levels.
        for (auto& nodes : m_levelSets)
        {
            nodes.clear();
        }

        CHECK_STACK_EMPTY();
        m_stack.emplace_back(&m_root);
        m_levelWeights.clear();

        while (!m_stack.empty())
        {
            TreemapNode* node = m_stack.back();
            m_stack.pop_back();

            if (node->m_children.empty())
            {
                AZ_Warning(
                    "Profiler::ImGuiTreemap", node->m_weight >= 0.f,
                    "Treemap node %s in treemap %s has a negative weight. Only weights >= 0.f are premitted.",
                    node->m_name.IsEmpty() ? "[unnamed]" : node->m_name.GetCStr(), m_name.GetCStr());
                if (node->m_weight < 0.f)
                {
                    // Clamp the node weight below to zero to ensure negative weights don't throw off the algorithm
                    node->m_weight = 0.f;
                }
            }
            else
            {
                node->m_weight = 0.f;

                if (node->m_tag == 0 || (m_currentMask & node->m_tag) > 0)
                {
                    for (TreemapNode& child : node->m_children)
                    {
                        child.m_level = node->m_level + 1;
                        child.m_parent = node;
                        m_stack.emplace_back(&child);
                    }
                }
            }

            if (!node->m_group.IsEmpty())
            {
                auto it = m_groups.find(node->m_group);
                if (it == m_groups.end())
                {
                    GroupConfig& config = m_groups.try_emplace(node->m_group).first->second;
                    NameToHueSaturation(node->m_group, config.m_hue, config.m_saturation);
                }
            }

            if (m_levelSets.size() < node->m_level + 1)
            {
                m_levelSets.resize(node->m_level + 1);
            }
            // Track this node in one of the top level vectors. For non-leaf nodes we'll need to
            // accumulate its weight to its parent after all children are accounted for.
            m_levelSets[node->m_level].emplace_back(node);
        }

        // At this point, we've visited every node in the tree and initialized for the weights of all
        // non-leaf nodes. Now we have to accumulate values for the intermediate nodes, starting from the
        // last level working our way to the front.
        // Note that levels[0] is a single node vector containing m_root so we skip this level in our
        // traversal (it has no parent).

        auto end = m_levelSets.rend() - 1;
        for (auto it = m_levelSets.rbegin(); it != end; ++it)
        {
            for (TreemapNode* node : *it)
            {
                if (node->m_tag == 0 || (m_currentMask & node->m_tag) > 0)
                {
                    node->m_parent->m_weight += node->m_weight;
                }
            }
        }

        // Weights are determined for the root node and every intermediate node, so we can now determine
        // the weight sums across all nodes of a given level. Notice that the sums for the leaf nodes have
        // already been accumulated at this point.
        m_levelWeights.resize(m_levelSets.size());
        int level = 0;
        for (const auto& nodes : m_levelSets)
        {
            float& sum = m_levelWeights[level];
            for (TreemapNode* node : nodes)
            {
                if (node->m_tag == 0 || (m_currentMask & node->m_tag) > 0)
                {
                    sum += node->m_weight;
                }
            }
            ++level;
        }
    }

    void ImGuiTreemapImpl::AssignColors()
    {
        // Here we determine the color for each node, taking into account any selection filters and cursor
        // hover state.

        if (!m_dirty)
        {
            return;
        }

        CHECK_STACK_EMPTY();

        for (TreemapNode& node : m_root.m_children)
        {
            m_stack.push_back(&node);
        }

        while (!m_stack.empty())
        {
            TreemapNode& node = *m_stack.back();
            m_stack.pop_back();

            float hue;
            float saturation;

            if (node.m_parent == &m_root)
            {
                // We're looking at one of the user-supplied root nodes. Use the name to determine chromaticity
                NameToHueSaturation(node.m_name, hue, saturation);
            }
            else
            {
                // We're an intermediate or leaf node, not marked by a highlighted group so simply derive
                // chromaticity from the parent node.

                hue = node.m_parent->m_hue;
                saturation = node.m_parent->m_saturation;
            }

            node.m_hue = hue;
            node.m_saturation = saturation;

            // The value in the HSV color is based on the depth of this node in the tree, remapped to the
            // [0.4, 0.8] range (subtract 1 from node level to ignore root level).
            node.m_value = 0.4f * static_cast<float>(node.m_level - 1) / m_levelWeights.size() + 0.4f;

            for (TreemapNode& child : node.m_children)
            {
                m_stack.push_back(&child);
            }
        }
    }

    // This is the function referred to as "worst" in the Squarified paper. The idea is to determine how the
    // element aspect ratio changes if an element is added to row. The grade refers to the worst aspect ratio
    // among existing elements in the row. The sum, min, and max values correspond to areas within the row.
    static double GradeRow(double sum, double min, double max, int extent)
    {
        // The multiplication and division order here is somewhat haphazard but done this way to improve precision
        return AZStd::max(extent / sum * extent / sum * max, sum / extent * sum / extent / min);
    }

    void ImGuiTreemapImpl::SquarifyChildren(TreemapNode& node)
    {
        // The paper indicates better layouts were produced when sorting entries in descending weight order
        AZStd::vector<TreemapNode*> children;
        children.reserve(node.m_children.size() + 1);
        for (TreemapNode& child : node.m_children)
        {
            if (child.m_tag == 0 || (m_currentMask & child.m_tag) > 0)
            {
                children.push_back(&child);
            }
        }

        // This dummy node at the end is needed to finalize the last row (which will be the last child node
        // occupying the row by itself).
        TreemapNode endSentinel;
        endSentinel.m_weight = 0.f;
        children.push_back(&endSentinel);

        AZStd::sort(
            children.begin(), children.end(),
            [](const TreemapNode* lhs, const TreemapNode* rhs)
            {
                return lhs->m_weight > rhs->m_weight;
            });

        // Shrink the frame corresponding to a node to ensure there's a 2 pixel gutter.
        int rowExtent[2] = { node.m_extent[0] - 4, node.m_extent[1] - 4 };
        int rowOffset[2] = { node.m_offset[0] + 2, node.m_offset[1] + 2 };
        bool horizontal = rowExtent[1] > rowExtent[0];
        // The "extent" here tracks the extent along the rows orientation. The row we lay out entries within could be oriented vertically
        // depending on the aspect ratio.
        int extent = horizontal ? rowExtent[0] : rowExtent[1];

        AZStd::vector<TreemapNode*> row;
        double rowArea = 0.f;
        double rowMinArea = 0.f;
        double rowMaxArea = 0.f;
        double grade = 0.f;

        // The aspect ratios are computed with respect to element areas, so compute those areas here
        double scale = rowExtent[0] * rowExtent[1] / node.m_weight;
        for (TreemapNode* child : children)
        {
            child->m_area = child->m_weight * scale;
        }

        for (auto it = children.begin(); it != children.end(); /* iterator conditionally incremented in loop body */)
        {
            TreemapNode& child = **it;

            // If the row is empty, unconditionally start a new row.
            if (row.empty())
            {
                row.push_back(&child);
                rowArea = child.m_area;
                rowMinArea = child.m_area;
                rowMaxArea = child.m_area;
                grade = GradeRow(rowArea, rowMinArea, rowMaxArea, extent);
                ++it;
                continue;
            }

            // Check if this node belongs in the current row, or if we should start a new one
            double gradeIfAdded =
                GradeRow(rowArea + child.m_area, AZStd::min(child.m_area, rowMinArea), AZStd::max(child.m_area, rowMaxArea), extent);

            if (gradeIfAdded < grade)
            {
                grade = gradeIfAdded;

                // Adding this node improves the aspect ratio (nudges it closer to 1, aka makes it more like
                // a square) so we should append it to the current row
                row.push_back(&child);
                rowArea += child.m_area;
                if (child.m_area < rowMinArea)
                {
                    rowMinArea = child.m_area;
                }
                if (child.m_area > rowMaxArea)
                {
                    rowMaxArea = child.m_area;
                }
                ++it;
                continue;
            }

            // We're starting a new row, which means we need to finalize the layout of the current row.

            // Extent in the direction perpendicular to the orientation of the row
            int secondaryExtent = static_cast<int>(rowArea / extent);

            int offset[2] = { rowOffset[0], rowOffset[1] };

            for (TreemapNode* rowNode : row)
            {
                if (secondaryExtent <= 1)
                {
                    // These nodes are too small to display
                    rowNode->m_area = 0.0;
                }

                int nodeExtent = static_cast<int>(rowNode->m_area / secondaryExtent);

                rowNode->m_offset[0] = offset[0];
                rowNode->m_offset[1] = offset[1];

                if (horizontal)
                {
                    rowNode->m_extent[0] = nodeExtent;
                    rowNode->m_extent[1] = secondaryExtent;
                    offset[0] += nodeExtent;
                }
                else
                {
                    rowNode->m_extent[1] = nodeExtent;
                    rowNode->m_extent[0] = secondaryExtent;
                    offset[1] += nodeExtent;
                }

                // Clamp node position within row top left boundary
                rowNode->m_offset[0] = AZStd::clamp(rowNode->m_offset[0], rowOffset[0], rowOffset[0] + rowExtent[0]);
                rowNode->m_offset[1] = AZStd::clamp(rowNode->m_offset[1], rowOffset[1], rowOffset[1] + rowExtent[1]);

                // Clamp node extent based on row bottom right boundary
                if (rowNode->m_offset[0] + rowNode->m_extent[0] > rowOffset[0] + rowExtent[0])
                {
                    rowNode->m_extent[0] = rowOffset[0] + rowExtent[0] - rowNode->m_offset[0];
                }
                if (rowNode->m_offset[1] + rowNode->m_extent[1] > rowOffset[1] + rowExtent[1])
                {
                    rowNode->m_extent[1] = rowOffset[1] + rowExtent[1] - rowNode->m_offset[1];
                }
            }

            if (horizontal)
            {
                rowExtent[1] -= secondaryExtent;
                rowOffset[1] += secondaryExtent;
            }
            else
            {
                rowExtent[0] -= secondaryExtent;
                rowOffset[0] += secondaryExtent;
            }

            horizontal = rowExtent[1] > rowExtent[0];
            extent = horizontal ? rowExtent[0] : rowExtent[1];

            row.clear();
            // NOTE: we don't increment the iterator here. Since this node will be used to initialize the next row.
        }
    }

    void ImGuiTreemapImpl::ComputeLayout(int w, int h)
    {
        if (m_lastExtent[0] == w && m_lastExtent[1] == h)
        {
            return;
        }

        // This function implements the "Squarified Treemap" algorithm as documented here:
        // https://www.win.tue.nl/~vanwijk/stm.pdf
        // (archive link: https://web.archive.org/web/20220224165104/https://www.win.tue.nl/~vanwijk/stm.pdf)
        // After function completion, every node will have a computed offset and extent

        m_root.m_offset[0] = 0;
        m_root.m_offset[1] = 0;
        m_root.m_extent[0] = w;
        m_root.m_extent[1] = h;

        // One modification to the paper implementation is that layout generation is done using a stack
        // instead of recursion
        CHECK_STACK_EMPTY();
        m_stack.push_back(&m_root);

        while (!m_stack.empty())
        {
            TreemapNode* node = m_stack.back();
            m_stack.pop_back();

            if (!node->m_children.empty())
            {
                SquarifyChildren(*node);

                for (TreemapNode& child : node->m_children)
                {
                    // Leaf nodes don't need to be pushed onto the stack
                    if (!child.m_children.empty())
                    {
                        if (node->m_tag == 0 || (m_currentMask & child.m_tag) > 0)
                        {
                            m_stack.push_back(&child);
                        }
                    }
                }
            }
        }

        m_lastExtent[0] = w;
        m_lastExtent[1] = h;
    }

} // namespace Profiler

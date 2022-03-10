/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Interface/Interface.h>
#include <imgui/imgui.h>

namespace Profiler
{
    // TreemapNodes support arbitrary child counts and nesting levels
    struct TreemapNode
    {
        // The name of the treemap node
        AZ::Name m_name;

        // Nodes may be classified by an additional group label, which may be used to color the node in the final visualization.
        // For example, you might want to highlight all blocks in a treemap that are a particular filetype, or show "Texture2D" 
        // allocations as the same color.
        AZ::Name m_group;

        // If left empty, a tooltip will be automatically generated containing the name and weight (with associated unit label)
        AZStd::string m_tooltip;

        // The weight must be positive definite and should only be specified *on the leaves* (e.g. m_children is nullptr)
        // The weight may be specified on parent nodes (e.g. to assist in weight normalization of the leaves) but note that
        // this value will be overwritten
        float m_weight = 0.f;

        // ADVANCED
        // The tag can be used to filter this node from displaying or not in the visualization. For example, we may tag all
        // unused memory regions as 0x1, and later use a mask of 0x0 to omit unused memory from the display. By default, all
        // nodes are shown (0xffffffff mask). To show select nodes, use the ImGuiTreemap::AddMask method to set a mask to a
        // particular label. The expression to determine if a node should be included or not is (mask & m_tag > 0). That is,
        // this node will be shown if any of the mask bits coincide with any of the tag bits.
        uint32_t m_tag = 0;

        // Count and pointer to an array of m_childCount children. May be left default-initialized for leaf nodes
        AZStd::vector<TreemapNode> m_children;

    private:
        // Private data is modified during treemap generation and not intended to be modified by the user
        friend class ImGuiTreemapImpl;

        TreemapNode* m_parent;
        double m_area = 0.f;
        float m_hue;
        float m_saturation;
        float m_value;
        int m_level = 0;
        int m_offset[2];
        int m_extent[2];
    };

    // A treemap is a 2D visualization of entries designed to emphasize relative size differences. It
    // is commonly used to visualize disk space utilization, but extends naturally to understanding
    // memory allocations, archive data, and more.
    class ImGuiTreemap
    {
    public:
        virtual ~ImGuiTreemap() = default;

        //! Retrieve the Treemap name
        virtual const AZ::Name& GetName() const = 0;

        //! Set Treemap name (used in UI titlebar display)
        virtual void SetName(char const* name) = 0;
        virtual void SetName(AZ::Name name) = 0;

        //! Set unit label (e.g. lbs, square footage, MB, etc.)
        virtual void SetUnitLabel(char const* unitLabel) = 0;

        //! Add a UI radio button that renders only nodes possessing a tag that is either 0 or passes the mask
        virtual void AddMask(const char* label, uint32_t mask) = 0;

        //! Supply the root nodes of the treemap.
        virtual void SetRoots(AZStd::vector<TreemapNode>&& roots) = 0;

        //! Submit ImGui directives to draw the treemap
        virtual void Render(int x, int y, int w, int h) = 0;

        //! Weigh entries and perform layout. This occurs automatically on `Render` but is exposed here if you wish
        //! to perform a layout in advance. Note that previously computed scores and the computed layout are cached
        //! until entries are modified, added, removed, or the window size is changed.
        virtual void WeighAndComputeLayout(int x, int y, int w, int h) = 0;
    };

    class ImGuiTreemapFactory
    {
    public:
        using Interface = AZ::Interface<ImGuiTreemapFactory>;

        AZ_RTTI(ImGuiTreemapFactory, "{90BCA753-6152-4942-8A81-DD14196A6811}");

        virtual ~ImGuiTreemapFactory() = default;

        virtual ImGuiTreemap& Create(AZ::Name name, const char* unitLabel) = 0;
        virtual void Destroy(ImGuiTreemap* treemap) = 0;
    };
} // namespace Profiler

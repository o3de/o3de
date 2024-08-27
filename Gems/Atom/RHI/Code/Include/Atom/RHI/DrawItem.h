/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DrawArguments.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/IndirectArguments.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    class PipelineState;
    class ShaderResourceGroup;
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template <typename T , typename NamespaceType>
    struct Handle;

    // A DrawItem corresponds to one draw of one mesh in one pass. Multiple draw items are bundled
    // in a DrawPacket, which corresponds to multiple draws of one mesh in multiple passes.
    // NOTE: Do not rely solely on default member initialization here, as DrawItems are bulk allocated for
    // DrawPackets and their memory aliased in DrawPacketBuilder. Any default values should also be specified
    // in the DrawPacketBuilder::End() function (see DrawPacketBuilder.cpp)
    struct DrawItem
    {
        DrawItem() = default;

        /// Indices of the StreamBufferViews in the GeometryView that this DrawItem will use
        RHI::GeometryView::StreamBufferIndices m_streamIndices;

        u8  m_stencilRef = 0;
        u8  m_shaderResourceGroupCount = 0;
        u8  m_rootConstantSize = 0;
        u8  m_scissorsCount = 0;
        u8  m_viewportsCount = 0;

        union
        {
            struct
            {
                // NOTE: If you add or update any of these flags, please update the default value of m_allFlags
                // so that your added flags are initialized properly. Also update the default value specified in
                // the DrawPacketBuilder::End() function (see DrawPacketBuilder.cpp). See comment above.

                bool m_enabled : 1;     // Whether the Draw Item should render
            };
            u8 m_allFlags = 1;     //< Update default value if you add flags. Also update in DrawPacketBuilder::End()
        };

        // --- Geometry ---

        /// Contains DrawArguments and geometry buffer views used during rendering
        const GeometryView* m_geometryView = nullptr;

        // --- Shader ---

        const PipelineState* m_pipelineState = nullptr;

        /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        const ShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

        /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
        const ShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

        /// Array of root constants to bind (count must match m_rootConstantSize).
        const u8* m_rootConstants = nullptr;

        // --- Scissor and Viewport ---

        /// List of scissors to be applied to this draw item only. Scissor will be restore to the previous state
        /// after the DrawItem has been processed.
        const Scissor* m_scissors = nullptr;

        /// List of viewports to be applied to this draw item only. Viewports will be restore to the previous state
        /// after the DrawItem has been processed.
        const Viewport* m_viewports = nullptr;
    };

    using DrawItemSortKey = AZ::s64;

    // A filter associate to a DrawItem which can be used to filter the DrawItem when submitting to command list
    using DrawFilterTag = Handle<u8, DefaultNamespaceType>;
    using DrawFilterMask = u32; // AZStd::bitset's impelmentation is too expensive.
    constexpr u32 DrawFilterMaskDefaultValue = u32(-1);  // Default all bit to 1.
    static_assert(sizeof(DrawFilterMask) * 8 >= Limits::Pipeline::DrawFilterTagCountMax, "DrawFilterMask doesn't have enough bits for maximum tag count");

    struct DrawItemProperties
    {
        bool operator==(const DrawItemProperties& rhs) const
        {
            return m_item == rhs.m_item &&
                m_sortKey == rhs.m_sortKey &&
                m_depth == rhs.m_depth &&
                m_drawFilterMask == rhs.m_drawFilterMask
                ;
        }

        bool operator!=(const DrawItemProperties& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const DrawItemProperties& rhs) const
        {
            return m_sortKey < rhs.m_sortKey;
        }

        //! A pointer to the draw item
        const DrawItem* m_item = nullptr;
        //! A sorting key of this draw item which is used for sorting draw items in DrawList
        //! Check RHI::SortDrawList() function for detail
        DrawItemSortKey m_sortKey = 0;
        //! A filter mask which helps decide whether to submit this draw item to a Scope's command list or not
        DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
        //! A depth value this draw item which is used for sorting draw items in DrawList
        //! Check RHI::SortDrawList() function for detail
        float m_depth = 0.0f;
    };
}

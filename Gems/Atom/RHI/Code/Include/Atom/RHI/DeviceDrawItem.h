/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDrawArguments.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <Atom/RHI/DeviceIndirectBufferView.h>
#include <Atom/RHI/DeviceIndirectArguments.h>
#include <Atom/RHI/DeviceStreamBufferView.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    class DevicePipelineState;
    class DeviceShaderResourceGroup;
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template <typename T , typename NamespaceType>
    struct Handle;

    // A DeviceDrawItem corresponds to one draw of one mesh in one pass. Multiple draw items are bundled
    // in a DeviceDrawPacket, which corresponds to multiple draws of one mesh in multiple passes.
    // NOTE: Do not rely solely on default member initialization here, as DrawItems are bulk allocated for
    // DrawPackets and their memory aliased in DeviceDrawPacketBuilder. Any default values should also be specified
    // in the DeviceDrawPacketBuilder::End() function (see DeviceDrawPacketBuilder.cpp)
    struct DeviceDrawItem
    {
        DeviceDrawItem() = default;

        DrawInstanceArguments m_drawInstanceArgs;

        /// Indices of the StreamBufferViews in the GeometryView that this DrawItem will use
        StreamBufferIndices m_streamIndices;
        uint8_t m_stencilRef = 0;
        uint8_t m_shaderResourceGroupCount = 0;
        uint8_t m_rootConstantSize = 0;
        uint8_t m_scissorsCount = 0;
        uint8_t m_viewportsCount = 0;

        union
        {
            struct
            {
                // NOTE: If you add or update any of these flags, please update the default value of m_allFlags
                // so that your added flags are initialized properly. Also update the default value specified in
                // the DeviceDrawPacketBuilder::End() function (see DeviceDrawPacketBuilder.cpp). See comment above.

                bool m_enabled : 1;     // Whether the Draw Item should render
            };
            uint8_t m_allFlags = 1;     //< Update default value if you add flags. Also update in DeviceDrawPacketBuilder::End()
        };

        // --- Geometry ---

        /// The GeometryView used when drawing with an indexed draw call.
        const DeviceGeometryView* m_geometryView = nullptr;

        // --- Shader ---

        const DevicePipelineState* m_pipelineState = nullptr;

        /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        const DeviceShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

        /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
        const DeviceShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

        /// Array of root constants to bind (count must match m_rootConstantSize).
        const uint8_t* m_rootConstants = nullptr;

        // --- Scissor and Viewport ---

        /// List of scissors to be applied to this draw item only. Scissor will be restore to the previous state
        /// after the DeviceDrawItem has been processed.
        const Scissor* m_scissors = nullptr;

        /// List of viewports to be applied to this draw item only. Viewports will be restore to the previous state
        /// after the DeviceDrawItem has been processed.
        const Viewport* m_viewports = nullptr;
    };

    using DrawItemSortKey = AZ::s64;

    // A filter associate to a DeviceDrawItem which can be used to filter the DeviceDrawItem when submitting to command list
    using DrawFilterTag = Handle<uint8_t, DefaultNamespaceType>;
    // This mask is typically used to filter out DrawItems that should or shouldn't be submitted
    // to a particular RenderPipeline.
    // A RenderPipeline builds it DrawFilterMask using two bits (Tags):
    // 1- Tag from its m_nameId
    // 2- Tag from its m_materialPipelineTagName
    // On the other hand, a DrawItem has two options:
    // 1- If it doesn't come from a shader listed under a MaterialPipeline, then all bits are enabled.
    //    Which means the DrawItem will be valid for all active RenderPipelines.
    // 2- If it comes from a shader listed under a MaterialPipeline, then only the bit of the given
    //    MaterialPipelineTag is enabled.
    using DrawFilterMask = uint32_t; // AZStd::bitset's impelmentation is too expensive.
    constexpr uint32_t DrawFilterMaskDefaultValue = uint32_t(-1);  // Default all bit to 1.
    static_assert(sizeof(DrawFilterMask) * 8 >= Limits::Pipeline::DrawFilterTagCountMax, "DrawFilterMask doesn't have enough bits for maximum tag count");

    struct DeviceDrawItemProperties
    {
        bool operator==(const DeviceDrawItemProperties& rhs) const
        {
            return m_item == rhs.m_item &&
                m_sortKey == rhs.m_sortKey &&
                m_depth == rhs.m_depth &&
                m_drawFilterMask == rhs.m_drawFilterMask
                ;
        }

        bool operator!=(const DeviceDrawItemProperties& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const DeviceDrawItemProperties& rhs) const
        {
            return m_sortKey < rhs.m_sortKey;
        }

        //! A pointer to the draw item
        const DeviceDrawItem* m_item = nullptr;
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

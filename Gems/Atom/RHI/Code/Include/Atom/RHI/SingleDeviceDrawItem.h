/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/SingleDeviceStreamBufferView.h>
#include <Atom/RHI/SingleDeviceIndexBufferView.h>
#include <Atom/RHI/SingleDeviceIndirectBufferView.h>
#include <Atom/RHI/SingleDeviceIndirectArguments.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    class SingleDevicePipelineState;
    class SingleDeviceShaderResourceGroup;
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template <typename T , typename NamespaceType>
    struct Handle;

    struct DrawLinear
    {
        DrawLinear() = default;

        DrawLinear(
            uint32_t instanceCount,
            uint32_t instanceOffset,
            uint32_t vertexCount,
            uint32_t vertexOffset)
            : m_instanceCount(instanceCount)
            , m_instanceOffset(instanceOffset)
            , m_vertexCount(vertexCount)
            , m_vertexOffset(vertexOffset)
        {}

        uint32_t m_instanceCount = 1;
        uint32_t m_instanceOffset = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_vertexOffset = 0;
    };

    struct DrawIndexed
    {
        DrawIndexed() = default;

        DrawIndexed(
            uint32_t instanceCount,
            uint32_t instanceOffset,
            uint32_t vertexOffset,
            uint32_t indexCount,
            uint32_t indexOffset)
            : m_instanceCount(instanceCount)
            , m_instanceOffset(instanceOffset)
            , m_vertexOffset(vertexOffset)
            , m_indexCount(indexCount)
            , m_indexOffset(indexOffset)
        {}

        uint32_t m_instanceCount = 1;
        uint32_t m_instanceOffset = 0;
        uint32_t m_vertexOffset = 0;
        uint32_t m_indexCount = 0;
        uint32_t m_indexOffset = 0;
    };

    using DrawIndirect = SingleDeviceIndirectArguments;

    enum class DrawType : uint8_t
    {
        Indexed = 0,
        Linear,
        Indirect
    };

    struct SingleDeviceDrawArguments
    {
        AZ_TYPE_INFO(SingleDeviceDrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        SingleDeviceDrawArguments() : SingleDeviceDrawArguments(DrawIndexed{}) {}

        SingleDeviceDrawArguments(const DrawIndexed& indexed)
            : m_type{DrawType::Indexed}
            , m_indexed{indexed}
        {}

        SingleDeviceDrawArguments(const DrawLinear& linear)
            : m_type{DrawType::Linear}
            , m_linear{linear}
        {}


        SingleDeviceDrawArguments(const DrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_indirect{ indirect }
        {}

        DrawType m_type;
        union
        {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            DrawIndirect m_indirect;
        };
    };

    // A SingleDeviceDrawItem corresponds to one draw of one mesh in one pass. Multiple draw items are bundled
    // in a SingleDeviceDrawPacket, which corresponds to multiple draws of one mesh in multiple passes.
    // NOTE: Do not rely solely on default member initialization here, as DrawItems are bulk allocated for
    // DrawPackets and their memory aliased in SingleDeviceDrawPacketBuilder. Any default values should also be specified
    // in the SingleDeviceDrawPacketBuilder::End() function (see SingleDeviceDrawPacketBuilder.cpp)
    struct SingleDeviceDrawItem
    {
        SingleDeviceDrawItem() = default;

        SingleDeviceDrawArguments m_arguments;

        uint8_t m_stencilRef = 0;
        uint8_t m_streamBufferViewCount = 0;
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
                // the SingleDeviceDrawPacketBuilder::End() function (see SingleDeviceDrawPacketBuilder.cpp). See comment above.

                bool m_enabled : 1;     // Whether the Draw Item should render
            };
            uint8_t m_allFlags = 1;     //< Update default value if you add flags. Also update in SingleDeviceDrawPacketBuilder::End()
        };

        const SingleDevicePipelineState* m_pipelineState = nullptr;

        /// The index buffer used when drawing with an indexed draw call.
        const SingleDeviceIndexBufferView* m_indexBufferView = nullptr;

        /// Array of stream buffers to bind (count must match m_streamBufferViewCount).
        const SingleDeviceStreamBufferView* m_streamBufferViews = nullptr;

        /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        const SingleDeviceShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

        /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
        const SingleDeviceShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

        /// Array of root constants to bind (count must match m_rootConstantSize).
        const uint8_t* m_rootConstants = nullptr;

        /// List of scissors to be applied to this draw item only. Scissor will be restore to the previous state
        /// after the SingleDeviceDrawItem has been processed.
        const Scissor* m_scissors = nullptr;

        /// List of viewports to be applied to this draw item only. Viewports will be restore to the previous state
        /// after the SingleDeviceDrawItem has been processed.
        const Viewport* m_viewports = nullptr;
    };

    using DrawItemSortKey = AZ::s64;

    // A filter associate to a SingleDeviceDrawItem which can be used to filter the SingleDeviceDrawItem when submitting to command list
    using DrawFilterTag = Handle<uint8_t, DefaultNamespaceType>;
    using DrawFilterMask = uint32_t; // AZStd::bitset's impelmentation is too expensive.
    constexpr uint32_t DrawFilterMaskDefaultValue = uint32_t(-1);  // Default all bit to 1.
    static_assert(sizeof(DrawFilterMask) * 8 >= Limits::Pipeline::DrawFilterTagCountMax, "DrawFilterMask doesn't have enough bits for maximum tag count");

    struct SingleDeviceDrawItemProperties
    {
        bool operator==(const SingleDeviceDrawItemProperties& rhs) const
        {
            return m_item == rhs.m_item &&
                m_sortKey == rhs.m_sortKey &&
                m_depth == rhs.m_depth &&
                m_drawFilterMask == rhs.m_drawFilterMask
                ;
        }

        bool operator!=(const SingleDeviceDrawItemProperties& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const SingleDeviceDrawItemProperties& rhs) const
        {
            return m_sortKey < rhs.m_sortKey;
        }

        //! A pointer to the draw item
        const SingleDeviceDrawItem* m_item = nullptr;
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

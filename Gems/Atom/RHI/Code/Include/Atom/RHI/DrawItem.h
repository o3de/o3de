/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/IndirectArguments.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineState;
        class ShaderResourceGroup;
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

        using DrawIndirect = IndirectArguments;

        enum class DrawType : uint8_t
        {
            Indexed = 0,
            Linear,
            Indirect
        };

        struct DrawArguments
        {
            AZ_TYPE_INFO(DrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

            DrawArguments() : DrawArguments(DrawIndexed{}) {}

            DrawArguments(const DrawIndexed& indexed)
                : m_type{DrawType::Indexed}
                , m_indexed{indexed}
            {}

            DrawArguments(const DrawLinear& linear)
                : m_type{DrawType::Linear}
                , m_linear{linear}
            {}


            DrawArguments(const DrawIndirect& indirect)
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

        struct DrawItem
        {
            DrawItem() = default;

            DrawArguments m_arguments;

            uint8_t m_stencilRef = 0;
            uint8_t m_streamBufferViewCount = 0;
            uint8_t m_shaderResourceGroupCount = 0;
            uint8_t m_rootConstantSize = 0;
            uint8_t m_scissorsCount = 0;
            uint8_t m_viewportsCount = 0;

            const PipelineState* m_pipelineState = nullptr;

            /// The index buffer used when drawing with an indexed draw call.
            const IndexBufferView* m_indexBufferView = nullptr;

            /// Array of stream buffers to bind (count must match m_streamBufferViewCount).
            const StreamBufferView* m_streamBufferViews = nullptr;

            /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
            const ShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

            /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
            const ShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

            /// Array of root constants to bind (count must match m_rootConstantSize).
            const uint8_t* m_rootConstants = nullptr;

            /// List of scissors to be applied to this draw item only. Scissor will be restore to the previous state
            /// after the DrawItem has been processed.
            const Scissor* m_scissors = nullptr;

            /// List of viewports to be applied to this draw item only. Viewports will be restore to the previous state
            /// after the DrawItem has been processed.
            const Viewport* m_viewports = nullptr;
        };

        using DrawItemSortKey = int64_t;

        // A filter associate to a DrawItem which can be used to filter the DrawItem when submitting to command list
        using DrawFilterTag = Handle<uint8_t, DefaultNamespaceType>;
        using DrawFilterMask = uint32_t; // AZStd::bitset's impelmentation is too expensive.
        constexpr uint32_t DrawFilterMaskDefaultValue = uint32_t(-1);  // Default all bit to 1.
        static_assert(sizeof(DrawFilterMask) * 8 >= Limits::Pipeline::DrawFilterTagCountMax, "DrawFilterMask doesn't have enough bits for maximum tag count");

        struct DrawItemProperties
        {
            DrawItemProperties() = default;

            DrawItemProperties(const DrawItem* item, DrawItemSortKey sortKey = 0, DrawFilterMask filterMask = DrawFilterMaskDefaultValue)
                : m_item{item}
                , m_sortKey{sortKey}
                , m_drawFilterMask{filterMask}
            {
            }

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
            // Check RHI::SortDrawList() function for detail
            DrawItemSortKey m_sortKey = 0;
            //! A depth value this draw item which is used for sorting draw items in DrawList
            //! Check RHI::SortDrawList() function for detail
            float m_depth = 0.0f;
            //! A filter mask which helps decide whether to submit this draw item to a Scope's command list or not
            DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
        };

    }
}

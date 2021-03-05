/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

            /// Array of inline constants to bind (count must match m_rootConstantSize).
            const uint8_t* m_rootConstants = nullptr;

            /// List of scissors to be applied to this draw item only. Scissor will be restore to the previous state
            /// after the DrawItem has been processed.
            const Scissor* m_scissors = nullptr;

            /// List of viewports to be applied to this draw item only. Viewports will be restore to the previous state
            /// after the DrawItem has been processed.
            const Viewport* m_viewports = nullptr;
        };

        using DrawItemSortKey = int64_t;

        struct DrawItemKeyPair
        {
            DrawItemKeyPair() = default;

            DrawItemKeyPair(const DrawItem* item, DrawItemSortKey sortKey)
                : m_item{item}
                , m_sortKey{sortKey}
            {}

            bool operator == (const DrawItemKeyPair& rhs) const
            {
                return m_item == rhs.m_item &&
                    m_sortKey == rhs.m_sortKey &&
                    m_depth == rhs.m_depth;
            }

            bool operator != (const DrawItemKeyPair& rhs) const
            {
                return !(*this == rhs);
            }

            bool operator < (const DrawItemKeyPair& rhs) const
            {
                return m_sortKey < rhs.m_sortKey;
            }

            const DrawItem* m_item = nullptr;
            DrawItemSortKey m_sortKey = 0;
            float m_depth = 0.0f;
        };

    }
}

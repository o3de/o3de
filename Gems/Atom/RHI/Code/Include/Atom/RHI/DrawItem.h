/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroup;
        struct Scissor;
        struct Viewport;
        struct DefaultNamespaceType;
        // Forward declaration to
        template<typename T, typename NamespaceType>
        struct Handle;

        using DrawIndirect = IndirectArguments;

        struct DrawArguments
        {
            AZ_TYPE_INFO(DrawArguments, "5E2B16AA-6BEA-4817-A0EA-A1701666B1C2");

            DrawArguments()
                : DrawArguments(DrawIndexed{})
            {
            }

            DrawArguments(const DrawIndexed& indexed)
                : m_type{ DrawType::Indexed }
                , m_indexed{ indexed }
            {}

            DrawArguments(const DrawLinear& linear)
                : m_type{ DrawType::Linear }
                , m_linear{ linear }
            {
            }

            DrawArguments(const DrawIndirect& indirect)
                : m_type{ DrawType::Indirect }
                , m_indirect{ indirect }
            {
            }

            DeviceDrawArguments GetDeviceDrawArguments(int deviceIndex) const
            {
                switch (m_type)
                {
                case DrawType::Indexed:
                    return DeviceDrawArguments(m_indexed);
                case DrawType::Linear:
                    return DeviceDrawArguments(m_linear);
                case DrawType::Indirect:
                    return DeviceDrawArguments(m_indirect.GetDeviceIndirectArguments(deviceIndex));
                default:
                    return DeviceDrawArguments();
                }
            }

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

            DeviceDrawItem GetDeviceDrawItem(
                int deviceIndex,
                DeviceIndexBufferView* indexBufferView,
                DeviceStreamBufferView* streamBufferViews,
                const DeviceShaderResourceGroup** shaderResourceGroups) const
            {
                DeviceDrawItem result;

                result.m_arguments = m_arguments.GetDeviceDrawArguments(deviceIndex);

                result.m_stencilRef = m_stencilRef;
                result.m_streamBufferViewCount = m_streamBufferViewCount;
                result.m_shaderResourceGroupCount = m_shaderResourceGroupCount;
                result.m_rootConstantSize = m_rootConstantSize;
                result.m_scissorsCount = m_scissorsCount;
                result.m_viewportsCount = m_viewportsCount;

                if (m_pipelineState)
                {
                    result.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
                }

                if (m_indexBufferView && indexBufferView)
                {
                    if (m_indexBufferView->GetByteCount())
                        *indexBufferView = m_indexBufferView->GetDeviceIndexBufferView(deviceIndex);
                    result.m_indexBufferView = indexBufferView;
                }

                if (m_streamBufferViews && streamBufferViews)
                {
                    for (int i = 0; i < m_streamBufferViewCount; ++i)
                    {
                        if (m_streamBufferViews[i].GetByteCount())
                            streamBufferViews[i] = m_streamBufferViews[i].GetDeviceStreamBufferView(deviceIndex);
                    }

                    result.m_streamBufferViews = streamBufferViews;
                }

                if (m_shaderResourceGroups && shaderResourceGroups)
                {
                    for (int i = 0; i < m_shaderResourceGroupCount; ++i)
                    {
                        shaderResourceGroups[i] = m_shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                    }

                    result.m_shaderResourceGroups = shaderResourceGroups;
                }

                if (m_uniqueShaderResourceGroup)
                {
                    result.m_uniqueShaderResourceGroup = m_uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                result.m_rootConstants = m_rootConstants;
                result.m_scissors = m_scissors;
                result.m_viewports = m_viewports;

                return result;
            }

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

        struct DrawItemProperties
        {
            DrawItemProperties() = default;

            DrawItemProperties(const DrawItem* item, DrawItemSortKey sortKey = 0, DrawFilterMask filterMask = DrawFilterMaskDefaultValue)
                : m_item{ item }
                , m_sortKey{ sortKey }
                , m_drawFilterMask{ filterMask }
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

            DeviceDrawItemProperties GetDeviceDrawItemProperties(
                int deviceIndex,
                DeviceDrawItem* drawItem,
                DeviceIndexBufferView* indexBufferView,
                DeviceStreamBufferView* streamBufferViews,
                const DeviceShaderResourceGroup** shaderResourceGroups) const
            {
                *drawItem = m_item->GetDeviceDrawItem(deviceIndex, indexBufferView, streamBufferViews, shaderResourceGroups);
                DeviceDrawItemProperties result{ drawItem, m_sortKey, m_drawFilterMask };
                result.m_depth = m_depth;
                return result;
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

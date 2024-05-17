/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/SingleDeviceDrawItem.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <Atom/RHI/MultiDeviceIndirectArguments.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/MultiDeviceStreamBufferView.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template<typename T, typename NamespaceType>
    struct Handle;

    using MultiDeviceDrawIndirect = MultiDeviceIndirectArguments;

    //! A structure used to define the type of draw that should happen, directly passed on to the device-specific DrawItems in
    //! MultiDeviceDrawItem::SetArguments
    struct MultiDeviceDrawArguments
    {
        AZ_TYPE_INFO(MultiDeviceDrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        MultiDeviceDrawArguments()
            : MultiDeviceDrawArguments(DrawIndexed{})
        {
        }

        MultiDeviceDrawArguments(const DrawIndexed& indexed)
            : m_type{ DrawType::Indexed }
            , m_indexed{ indexed }
        {
        }

        MultiDeviceDrawArguments(const DrawLinear& linear)
            : m_type{ DrawType::Linear }
            , m_linear{ linear }
        {
        }

        MultiDeviceDrawArguments(const MultiDeviceDrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_mdIndirect{ indirect }
        {
        }

        //! Returns the device-specific SingleDeviceDrawArguments for the given index
        SingleDeviceDrawArguments GetDeviceDrawArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DrawType::Indexed:
                return SingleDeviceDrawArguments(m_indexed);
            case DrawType::Linear:
                return SingleDeviceDrawArguments(m_linear);
            case DrawType::Indirect:
                return SingleDeviceDrawArguments(DrawIndirect{m_mdIndirect.m_maxSequenceCount, m_mdIndirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex), m_mdIndirect.m_indirectBufferByteOffset, m_mdIndirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get(), m_mdIndirect.m_countBufferByteOffset});
            default:
                return SingleDeviceDrawArguments();
            }
        }

        DrawType m_type;
        union {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            MultiDeviceDrawIndirect m_mdIndirect;
        };
    };

    class MultiDeviceDrawItem
    {
        friend class MultiDeviceDrawPacketBuilder;

    public:
        MultiDeviceDrawItem(MultiDevice::DeviceMask deviceMask);

        MultiDeviceDrawItem(MultiDevice::DeviceMask deviceMask, AZStd::unordered_map<int, SingleDeviceDrawItem*>&& deviceDrawItemPtrs);

        MultiDeviceDrawItem(const MultiDeviceDrawItem& other) = delete;
        MultiDeviceDrawItem(MultiDeviceDrawItem&& other) = default;

        MultiDeviceDrawItem& operator=(const MultiDeviceDrawItem& other) = delete;
        MultiDeviceDrawItem& operator=(MultiDeviceDrawItem&& other) = default;

        //! Returns the device-specific SingleDeviceDrawItem for the given index
        const SingleDeviceDrawItem& GetDeviceDrawItem(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceDrawItem",
                m_deviceDrawItemPtrs.find(deviceIndex) != m_deviceDrawItemPtrs.end(),
                "No DeviceDrawItem found for device index %d\n",
                deviceIndex);

            return *m_deviceDrawItemPtrs.at(deviceIndex);
        }

        bool GetEnabled() const
        {
            return m_enabled;
        }

        void SetEnabled(bool enabled)
        {
            m_enabled = enabled;

            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_enabled = enabled;
            }
        }

        void SetArguments(const MultiDeviceDrawArguments& arguments)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_arguments = arguments.GetDeviceDrawArguments(deviceIndex);
            }
        }

        void SetIndexedArgumentsInstanceCount(uint32_t instanceCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_arguments.m_indexed.m_instanceCount = instanceCount;
            }
        }

        void SetStencilRef(uint8_t stencilRef)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_stencilRef = stencilRef;
            }
        }

        void SetPipelineState(const MultiDevicePipelineState* pipelineState)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_pipelineState = pipelineState->GetDevicePipelineState(deviceIndex).get();
            }
        }

        //! The index buffer used when drawing with an indexed draw call.
        void SetIndexBufferView(const MultiDeviceIndexBufferView* indexBufferView)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                m_deviceIndexBufferView.emplace(deviceIndex, indexBufferView->GetDeviceIndexBufferView(deviceIndex));
            }

            // Done extra so memory is not moved around any more during map resize
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_indexBufferView = &m_deviceIndexBufferView[deviceIndex];
            }
        }

        //! Array of stream buffers to bind (count must match m_streamBufferViewCount).
        void SetStreamBufferViews(const MultiDeviceStreamBufferView* streamBufferViews, uint32_t streamBufferViewCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_streamBufferViewCount = static_cast<uint8_t>(streamBufferViewCount);

                auto [it, insertOK]{ m_deviceStreamBufferViews.emplace(deviceIndex, AZStd::vector<SingleDeviceStreamBufferView>{}) };

                auto& [index, deviceStreamBufferView]{ *it };

                for (auto i = 0u; i < streamBufferViewCount; ++i)
                {
                    deviceStreamBufferView.emplace_back(streamBufferViews[i].GetDeviceStreamBufferView(deviceIndex));
                }

                drawItem->m_streamBufferViews = deviceStreamBufferView.data();
            }
        }

        //! Shader Resource Groups
        void SetShaderResourceGroups(const MultiDeviceShaderResourceGroup* const* shaderResourceGroups, uint32_t shaderResourceGroupCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_shaderResourceGroupCount = static_cast<uint8_t>(shaderResourceGroupCount);

                auto [it, insertOK]{ m_deviceShaderResourceGroups.emplace(
                    deviceIndex, AZStd::vector<SingleDeviceShaderResourceGroup*>(shaderResourceGroupCount)) };

                auto& [index, deviceShaderResourceGroup]{ *it };

                for (auto i = 0u; i < shaderResourceGroupCount; ++i)
                {
                    deviceShaderResourceGroup[i] = shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                drawItem->m_shaderResourceGroups = deviceShaderResourceGroup.data();
            }
        }

        //! Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback
        //! key
        void SetUniqueShaderResourceGroup(const MultiDeviceShaderResourceGroup* uniqueShaderResourceGroup)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_uniqueShaderResourceGroup = uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
        }

        //! Array of root constants to bind (count must match m_rootConstantSize).
        void SetRootConstants(const uint8_t* rootConstants, uint8_t rootConstantSize)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_rootConstantSize = rootConstantSize;
                drawItem->m_rootConstants = rootConstants;
            }
        }

        //! List of scissors to be applied to this draw item only. Scissor will be restored to the previous state
        //! after the MultiDeviceDrawItem has been processed.
        void SetScissors(const Scissor* scissors, uint8_t scissorsCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_scissorsCount = scissorsCount;
                drawItem->m_scissors = scissors;
            }
        }

        //! List of viewports to be applied to this draw item only. Viewports will be restored to the previous state
        //! after the MultiDeviceDrawItem has been processed.
        void SetViewports(const Viewport* viewports, uint8_t viewportCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_viewportsCount = viewportCount;
                drawItem->m_viewports = viewports;
            }
        }

    private:
        bool m_enabled{ true };
        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
        //! A map of all device-specific DrawItems, indexed by the device index
        AZStd::unordered_map<int, SingleDeviceDrawItem> m_deviceDrawItems;
        //! A map of pointers to device-specific DrawItems, indexed by the device index
        //! These pointers may point to m_deviceDrawItems (in case of direct usage of a SingleDeviceDrawItem)
        //! or may point to DrawItems in linear memory (when allocated via a SingleDeviceDrawPacket)
        AZStd::unordered_map<int, SingleDeviceDrawItem*> m_deviceDrawItemPtrs;
        //! A map of all device-specific IndexBufferViews, indexed by the device index
        //! This additional cache is needed since device-specific IndexBufferViews are returned as objects
        //! and the device-specific SingleDeviceDrawItem holds a pointer to it.
        AZStd::unordered_map<int, SingleDeviceIndexBufferView> m_deviceIndexBufferView;
        //! A map of all device-specific StreamBufferViews, indexed by the device index
        //! This additional cache is needed since device-specific StreamBufferViews are returned as objects
        //! and the device-specific SingleDeviceDrawItem holds a pointer to it.
        AZStd::unordered_map<int, AZStd::vector<SingleDeviceStreamBufferView>> m_deviceStreamBufferViews;
        //! A map of all device-specific ShaderResourceGroups, indexed by the device index
        //! This additional cache is needed since device-specific ShaderResourceGroups are provided as a SingleDeviceShaderResourceGroup**,
        //! which are then locally cached in a vector (per device) and the device-specific SingleDeviceDrawItem holds a pointer to this vector's data.
        AZStd::unordered_map<int, AZStd::vector<SingleDeviceShaderResourceGroup*>> m_deviceShaderResourceGroups;
    };

    struct MultiDeviceDrawItemProperties
    {
        bool operator==(const MultiDeviceDrawItemProperties& rhs) const
        {
            return m_mdItem == rhs.m_mdItem && m_sortKey == rhs.m_sortKey && m_depth == rhs.m_depth &&
                m_drawFilterMask == rhs.m_drawFilterMask;
        }

        bool operator!=(const MultiDeviceDrawItemProperties& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const MultiDeviceDrawItemProperties& rhs) const
        {
            return m_sortKey < rhs.m_sortKey;
        }

        //! Returns the device-specific SingleDeviceDrawItemProperties for the given index
        SingleDeviceDrawItemProperties GetDeviceDrawItemProperties(int deviceIndex) const
        {
            AZ_Assert(m_mdItem, "Not initialized with MultiDeviceDrawItem\n");

            SingleDeviceDrawItemProperties result{ &m_mdItem->GetDeviceDrawItem(deviceIndex), m_sortKey, m_drawFilterMask, m_depth };

            return result;
        }

        //! A pointer to the draw item
        const MultiDeviceDrawItem* m_mdItem = nullptr;
        //! A sorting key of this draw item which is used for sorting draw items in DrawList
        //! Check RHI::SortDrawList() function for detail
        DrawItemSortKey m_sortKey = 0;
        //! A filter mask which helps decide whether to submit this draw item to a Scope's command list or not
        DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
        //! A depth value this draw item which is used for sorting draw items in DrawList
        //! Check RHI::SortDrawList() function for detail
        float m_depth = 0.0f;
    };
} // namespace AZ::RHI

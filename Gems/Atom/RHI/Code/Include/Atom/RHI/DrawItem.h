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
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/StreamBufferView.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template<typename T, typename NamespaceType>
    struct Handle;

    struct DrawItem
    {
        friend class DrawPacketBuilder;

    public:
        DrawItem(MultiDevice::DeviceMask deviceMask);

        DrawItem(MultiDevice::DeviceMask deviceMask, AZStd::unordered_map<int, DeviceDrawItem*>&& deviceDrawItemPtrs);

        DrawItem(const DrawItem& other) = delete;
        DrawItem(DrawItem&& other) = default;

        DrawItem& operator=(const DrawItem& other) = delete;
        DrawItem& operator=(DrawItem&& other) = default;

        //! Returns the device-specific DeviceDrawItem for the given index
        const DeviceDrawItem& GetDeviceDrawItem(int deviceIndex) const
        {
            AZ_Error(
                "DrawItem",
                m_deviceDrawItemPtrs.find(deviceIndex) != m_deviceDrawItemPtrs.end(),
                "No DeviceDrawItem found for device index %d\n",
                deviceIndex);

            return *m_deviceDrawItemPtrs.at(deviceIndex);
        }

        bool GetEnabled() const
        {
            return m_enabled;
        }

        //! The pipeline state type is the same regardless of the device,
        //! so we query here the default device.
        PipelineStateType GetPipelineStateType() const
        {
            const auto& deviceDrawItem = GetDeviceDrawItem(MultiDevice::DefaultDeviceIndex);
            return deviceDrawItem.m_pipelineState->GetType();
        }

        void SetEnabled(bool enabled)
        {
            m_enabled = enabled;

            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_enabled = enabled;
            }
        }

        void SetStencilRef(uint8_t stencilRef)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_stencilRef = stencilRef;
            }
        }

        void SetPipelineState(const PipelineState* pipelineState)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_pipelineState = pipelineState ? pipelineState->GetDevicePipelineState(deviceIndex).get() : nullptr;
            }
        }

        //! Shader Resource Groups
        void SetShaderResourceGroups(const ShaderResourceGroup* const* shaderResourceGroups, uint32_t shaderResourceGroupCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_shaderResourceGroupCount = static_cast<uint8_t>(shaderResourceGroupCount);

                auto [it, insertOK]{ m_deviceShaderResourceGroups.emplace(
                    deviceIndex, AZStd::vector<DeviceShaderResourceGroup*>(shaderResourceGroupCount)) };

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
        void SetUniqueShaderResourceGroup(const ShaderResourceGroup* uniqueShaderResourceGroup)
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
        //! after the DrawItem has been processed.
        void SetScissors(const Scissor* scissors, uint8_t scissorsCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_scissorsCount = scissorsCount;
                drawItem->m_scissors = scissors;
            }
        }

        //! List of viewports to be applied to this draw item only. Viewports will be restored to the previous state
        //! after the DrawItem has been processed.
        void SetViewports(const Viewport* viewports, uint8_t viewportCount)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_viewportsCount = viewportCount;
                drawItem->m_viewports = viewports;
            }
        }

        void SetDrawInstanceArgs(const RHI::DrawInstanceArguments& drawInstanceArgs)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_drawInstanceArgs = drawInstanceArgs;
            }
        }

        void SetGeometryView(RHI::GeometryView* geometryView)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_geometryView = geometryView->GetDeviceGeometryView(deviceIndex);
            }
        }

        void SetStreamIndices(RHI::StreamBufferIndices streamIndices)
        {
            for (auto& [deviceIndex, drawItem] : m_deviceDrawItemPtrs)
            {
                drawItem->m_streamIndices = streamIndices;
            }
        }

    private:
        bool m_enabled{ true };

        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };

        //! A map of all device-specific DrawItems, indexed by the device index
        AZStd::unordered_map<int, DeviceDrawItem> m_deviceDrawItems;

        //! A map of pointers to device-specific DrawItems, indexed by the device index
        //! These pointers may point to m_deviceDrawItems (in case of direct usage of a DeviceDrawItem)
        //! or may point to DrawItems in linear memory (when allocated via a DrawPacket)
        AZStd::unordered_map<int, DeviceDrawItem*> m_deviceDrawItemPtrs;

        //! A map of all device-specific ShaderResourceGroups, indexed by the device index
        //! This additional cache is needed since device-specific ShaderResourceGroups are provided as a DeviceShaderResourceGroup**,
        //! which are then locally cached in a vector (per device) and the device-specific DeviceDrawItem holds a pointer to this vector's data.
        AZStd::unordered_map<int, AZStd::vector<DeviceShaderResourceGroup*>> m_deviceShaderResourceGroups;
    };

    struct DrawItemProperties
    {
        bool operator==(const DrawItemProperties& rhs) const
        {
            return m_item == rhs.m_item && m_sortKey == rhs.m_sortKey && m_depth == rhs.m_depth &&
                m_drawFilterMask == rhs.m_drawFilterMask;
        }

        bool operator!=(const DrawItemProperties& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const DrawItemProperties& rhs) const
        {
            return m_sortKey < rhs.m_sortKey;
        }

        //! Returns the device-specific DeviceDrawItemProperties for the given index
        DeviceDrawItemProperties GetDeviceDrawItemProperties(int deviceIndex) const
        {
            AZ_Assert(m_item, "Not initialized with DrawItem\n");

            DeviceDrawItemProperties result{ &m_item->GetDeviceDrawItem(deviceIndex), m_sortKey, m_drawFilterMask, m_depth };

            return result;
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
} // namespace AZ::RHI

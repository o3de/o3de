/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/LinearAllocator.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::RHI
{
    DeviceDrawPacketBuilder::DeviceDrawRequest DrawPacketBuilder::DrawRequest::
        GetDeviceDrawRequest(int deviceIndex)
    {
        return DeviceDrawPacketBuilder::DeviceDrawRequest{
                m_listTag,
                m_stencilRef,
                m_streamIndices,
                m_uniqueShaderResourceGroup ? m_uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get() : nullptr,
                m_pipelineState ? m_pipelineState->GetDevicePipelineState(deviceIndex).get() : nullptr,
                m_sortKey,
                m_drawFilterMask };
    }

    DrawPacketBuilder::DrawPacketBuilder(const DrawPacketBuilder& other)
    {
        *this = other;
    }

    DrawPacketBuilder& DrawPacketBuilder::operator=(const DrawPacketBuilder& other)
    {
        m_deviceMask = other.m_deviceMask;

        m_drawRequests = other.m_drawRequests;

        m_drawPacketInFlight = aznew DrawPacket;
        if (other.m_drawPacketInFlight)
        {
            m_drawPacketInFlight->m_drawListMask = other.m_drawPacketInFlight->m_drawListMask;
        }

        m_deviceDrawPacketBuilders = other.m_deviceDrawPacketBuilders;

        return *this;
    }

    void DrawPacketBuilder::Begin(IAllocator* allocator)
    {
        AZ_Error(
            "DrawPacketBuilder",
            m_deviceMask != MultiDevice::DeviceMask{ 0u },
            "DrawPacketBuilder not initialized");

        m_drawPacketInFlight = aznew DrawPacket;

        for (auto& [_, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.Begin(allocator);
        }
    }

    void DrawPacketBuilder::SetGeometryView(GeometryView* geometryView)
    {
        for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.SetGeometryView(geometryView->GetDeviceGeometryView(deviceIndex));
        }
    }

    void DrawPacketBuilder::SetDrawInstanceArguments(DrawInstanceArguments drawInstanceArgs)
    {
        for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.SetDrawInstanceArguments(drawInstanceArgs);
        }

    }

    void DrawPacketBuilder::SetRootConstants(AZStd::span<const uint8_t> rootConstants)
    {
        for (auto& [_, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.SetRootConstants(rootConstants);
        }
    }

    void DrawPacketBuilder::SetScissors(AZStd::span<const Scissor> scissors)
    {
        for (auto& [_, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.SetScissors(scissors);
        }
    }

    void DrawPacketBuilder::SetScissor(const Scissor& scissor)
    {
        SetScissors(AZStd::span<const Scissor>(&scissor, 1));
    }

    void DrawPacketBuilder::SetViewports(AZStd::span<const Viewport> viewports)
    {
        for (auto& [_, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            deviceDrawPacketBuilder.SetViewports(viewports);
        }
    }

    void DrawPacketBuilder::SetViewport(const Viewport& viewport)
    {
        SetViewports(AZStd::span<const Viewport>(&viewport, 1));
    }

    void DrawPacketBuilder::AddShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup)
    {
        if (shaderResourceGroup)
        {
            for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
            {
                deviceDrawPacketBuilder.AddShaderResourceGroup(shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }
    }

    void DrawPacketBuilder::AddDrawItem(DrawRequest& request)
    {
        if (request.m_listTag.IsValid())
        {
            m_drawRequests.push_back(request);
            m_drawPacketInFlight->m_drawListMask.set(request.m_listTag.GetIndex());
            for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
            {
                deviceDrawPacketBuilder.AddDrawItem(m_drawRequests.back().GetDeviceDrawRequest(deviceIndex));
            }
        }
        else
        {
            AZ_Warning("DeviceDrawPacketBuilder", false, "Attempted to add a draw item to draw packet with no draw list tag assigned. Skipping.");
        }
    }

    RHI::Ptr<DrawPacket> DrawPacketBuilder::End()
    {
        if (m_drawRequests.empty())
        {
            return nullptr;
        }

        for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            m_drawPacketInFlight->m_deviceDrawPackets[deviceIndex] = deviceDrawPacketBuilder.End();
        }

        m_drawPacketInFlight->m_drawListTags.resize_no_construct(m_drawRequests.size());
        m_drawPacketInFlight->m_drawFilterMasks.resize_no_construct(m_drawRequests.size());
        m_drawPacketInFlight->m_drawItemSortKeys.resize_no_construct(m_drawRequests.size());
        m_drawPacketInFlight->m_drawItems.reserve(m_drawRequests.size());

        // Setup single-device DrawItems
        for (auto drawItemIndex{ 0 }; drawItemIndex < m_drawRequests.size(); ++drawItemIndex)
        {
            AZStd::unordered_map<int, DeviceDrawItem*> deviceDrawItemPtrs;
            for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
            {
                deviceDrawItemPtrs.emplace(deviceIndex, m_drawPacketInFlight->m_deviceDrawPackets[deviceIndex]->GetDrawItem(drawItemIndex));
            }
            m_drawPacketInFlight->m_drawItems.emplace_back(DrawItem{ m_deviceMask, AZStd::move(deviceDrawItemPtrs) });
        }

        const AZStd::vector<DrawListTag>& disabledTags = RHISystemInterface::Get()->GetDrawListTagsDisabledByDefault();
        for (size_t i = 0; i < m_drawRequests.size(); ++i)
        {
            const auto& drawRequest = m_drawRequests[i];

            m_drawPacketInFlight->m_drawListTags[i] = drawRequest.m_listTag;
            m_drawPacketInFlight->m_drawFilterMasks[i] = drawRequest.m_drawFilterMask;
            m_drawPacketInFlight->m_drawItemSortKeys[i] = drawRequest.m_sortKey;

            bool drawListTagDisabled = false;
            for (const DrawListTag& disabledTag : disabledTags)
            {
                drawListTagDisabled = drawListTagDisabled || (drawRequest.m_listTag == disabledTag);
            }

            auto& drawItem = m_drawPacketInFlight->m_drawItems[i];
            drawItem.SetEnabled(!drawListTagDisabled);
        }

        m_drawRequests.clear();

        return AZStd::move(m_drawPacketInFlight);
    }

    RHI::Ptr<DrawPacket> DrawPacketBuilder::Clone(const DrawPacket* original)
    {
        m_drawPacketInFlight = aznew DrawPacket;

        auto drawRequestCount{ original->m_drawListTags.size() };
        m_drawPacketInFlight->m_drawListMask = original->m_drawListMask;
        m_drawPacketInFlight->m_drawListTags.resize_no_construct(drawRequestCount);
        m_drawPacketInFlight->m_drawFilterMasks.resize_no_construct(drawRequestCount);
        m_drawPacketInFlight->m_drawItemSortKeys.resize_no_construct(drawRequestCount);
        m_drawPacketInFlight->m_drawItems.reserve(drawRequestCount);

        for (auto i{ 0 }; i < drawRequestCount; ++i)
        {
            m_drawPacketInFlight->m_drawListTags[i] = original->m_drawListTags[i];
            m_drawPacketInFlight->m_drawFilterMasks[i] = original->m_drawFilterMasks[i];
            m_drawPacketInFlight->m_drawItemSortKeys[i] = original->m_drawItemSortKeys[i];
        }

        for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
        {
            m_drawPacketInFlight->m_deviceDrawPackets[deviceIndex] =
                deviceDrawPacketBuilder.Clone(original->m_deviceDrawPackets.at(deviceIndex).get());
        }

        // Setup single-device DrawItems
        for (auto drawItemIndex{ 0 }; drawItemIndex < drawRequestCount; ++drawItemIndex)
        {
            AZStd::unordered_map<int, DeviceDrawItem*> deviceDrawItemPtrs;
            for (auto& [deviceIndex, deviceDrawPacketBuilder] : m_deviceDrawPacketBuilders)
            {
                deviceDrawItemPtrs.emplace(deviceIndex, m_drawPacketInFlight->m_deviceDrawPackets[deviceIndex]->GetDrawItem(drawItemIndex));
            }
            m_drawPacketInFlight->m_drawItems.emplace_back(DrawItem{ m_deviceMask, AZStd::move(deviceDrawItemPtrs) });
        }

        return AZStd::move(m_drawPacketInFlight);
    }
} // namespace AZ::RHI

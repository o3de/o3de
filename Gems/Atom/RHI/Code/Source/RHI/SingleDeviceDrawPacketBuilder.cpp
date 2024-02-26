/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/SingleDeviceDrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/LinearAllocator.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::RHI
{
    void SingleDeviceDrawPacketBuilder::Begin(IAllocator* allocator)
    {
        m_allocator = allocator ? allocator : &AllocatorInstance<SystemAllocator>::Get();
    }

    void SingleDeviceDrawPacketBuilder::SetDrawArguments(const SingleDeviceDrawArguments& drawArguments)
    {
        m_drawArguments = drawArguments;
    }

    void SingleDeviceDrawPacketBuilder::SetIndexBufferView(const SingleDeviceIndexBufferView& indexBufferView)
    {
        m_indexBufferView = indexBufferView;
    }

    void SingleDeviceDrawPacketBuilder::SetRootConstants(AZStd::span<const uint8_t> rootConstants)
    {
        m_rootConstants = rootConstants;
    }

    void SingleDeviceDrawPacketBuilder::SetScissors(AZStd::span<const Scissor> scissors)
    {
        m_scissors = decltype(m_scissors)(scissors.begin(), scissors.end());
    }

    void SingleDeviceDrawPacketBuilder::SetScissor(const Scissor& scissor)
    {
        SetScissors(AZStd::span<const Scissor>(&scissor, 1));
    }

    void SingleDeviceDrawPacketBuilder::SetViewports(AZStd::span<const Viewport> viewports)
    {
        m_viewports = decltype(m_viewports)(viewports.begin(), viewports.end());
    }

    void SingleDeviceDrawPacketBuilder::SetViewport(const Viewport& viewport)
    {
        SetViewports(AZStd::span<const Viewport>(&viewport, 1));
    }

    void SingleDeviceDrawPacketBuilder::AddShaderResourceGroup(const SingleDeviceShaderResourceGroup* shaderResourceGroup)
    {
        if (Validation::IsEnabled())
        {
            for (size_t i = 0; i < m_shaderResourceGroups.size(); ++i)
            {
                if (m_shaderResourceGroups[i] == shaderResourceGroup)
                {
                    AZ_Warning("DrawPacketCompiler", false, "Duplicate SingleDeviceShaderResourceGroup added to draw packet.");
                    return;
                }
            }
        }

        if (shaderResourceGroup)
        {
            m_shaderResourceGroups.push_back(shaderResourceGroup);
        }
    }

    void SingleDeviceDrawPacketBuilder::AddDrawItem(const SingleDeviceDrawRequest& request)
    {
        if (request.m_listTag.IsValid())
        {
            m_drawRequests.push_back(request);
            m_drawListMask.set(request.m_listTag.GetIndex());
            m_streamBufferViewCount += request.m_streamBufferViews.size();
        }
        else
        {
            AZ_Warning("SingleDeviceDrawPacketBuilder", false, "Attempted to add a draw item to draw packet with no draw list tag assigned. Skipping.");
        }
    }

    SingleDeviceDrawPacket* SingleDeviceDrawPacketBuilder::End()
    {
        if (m_drawRequests.empty())
        {
            return nullptr;
        }

        /**
            * This method constructs a single contiguous allocation big enough to fit all of the variable length arrays
            * stored in the draw items. The draw packet class occupies the initial part of the allocation, and is placement
            * new'd onto the larger allocation. The arrays are copied as POD types into the remaining payload. Delete is
            * overloaded on the draw packet to release the memory using the provided allocator instance.
            */

        // Alignment is specified per allocation. The size is unbounded.
        LinearAllocator::Descriptor linearAllocatorDesc;
        linearAllocatorDesc.m_alignmentInBytes = 1;
        linearAllocatorDesc.m_capacityInBytes = std::numeric_limits<size_t>::max();

        LinearAllocator linearAllocator;
        linearAllocator.Init(linearAllocatorDesc);

        [[maybe_unused]] const VirtualAddress drawPacketOffset = linearAllocator.Allocate(
            sizeof(SingleDeviceDrawPacket),
            AZStd::alignment_of<SingleDeviceDrawPacket>::value);

        const VirtualAddress drawItemsOffset = linearAllocator.Allocate(
            sizeof(SingleDeviceDrawItem) * m_drawRequests.size(),
            AZStd::alignment_of<SingleDeviceDrawItem>::value);

        const VirtualAddress drawItemSortKeysOffset = linearAllocator.Allocate(
            sizeof(DrawItemSortKey) * m_drawRequests.size(),
            AZStd::alignment_of<DrawItemSortKey>::value);

        const VirtualAddress drawListTagsOffset = linearAllocator.Allocate(
            sizeof(DrawListTag) * m_drawRequests.size(),
            AZStd::alignment_of<DrawListTag>::value);

        const VirtualAddress drawFilterMasksOffset = linearAllocator.Allocate(
            sizeof(DrawFilterMask) * m_drawRequests.size(),
            AZStd::alignment_of<DrawFilterMask>::value);

        const VirtualAddress shaderResourceGroupsOffset = linearAllocator.Allocate(
            sizeof(const SingleDeviceShaderResourceGroup*) * m_shaderResourceGroups.size(),
            AZStd::alignment_of<const SingleDeviceShaderResourceGroup*>::value);

        const VirtualAddress uniqueShaderResourceGroupsOffset = linearAllocator.Allocate(
            sizeof(const SingleDeviceShaderResourceGroup*) * m_drawRequests.size(),
            AZStd::alignment_of<const SingleDeviceShaderResourceGroup*>::value);

        const VirtualAddress rootConstantsOffset = linearAllocator.Allocate(
            sizeof(uint8_t) * m_rootConstants.size(),
            AZStd::alignment_of<uint8_t>::value);

        const VirtualAddress streamBufferViewsOffset = linearAllocator.Allocate(
            sizeof(SingleDeviceStreamBufferView) * m_streamBufferViewCount,
            AZStd::alignment_of<SingleDeviceStreamBufferView>::value);

        const VirtualAddress scissorOffset = linearAllocator.Allocate(
            sizeof(Scissor) * m_scissors.size(),
            AZStd::alignment_of<Scissor>::value);

        const VirtualAddress viewportOffset = linearAllocator.Allocate(
            sizeof(Viewport) * m_viewports.size(),
            AZStd::alignment_of<Viewport>::value);

        const size_t allocationSize = linearAllocator.GetAllocatedByteCount();
        auto allocationData = reinterpret_cast<uint8_t*>(m_allocator->Allocate(allocationSize, AZStd::alignment_of<SingleDeviceDrawPacket>::value));

        auto drawPacket = new (allocationData) SingleDeviceDrawPacket();
        drawPacket->m_allocator = m_allocator;
        drawPacket->m_indexBufferView =  m_indexBufferView;
        drawPacket->m_drawListMask = m_drawListMask;

        if (shaderResourceGroupsOffset.IsValid())
        {
            auto shaderResourceGroups = reinterpret_cast<const SingleDeviceShaderResourceGroup**>(allocationData + shaderResourceGroupsOffset.m_ptr);
            for (size_t i = 0; i < m_shaderResourceGroups.size(); ++i)
            {
                shaderResourceGroups[i] = m_shaderResourceGroups[i];
            }

            drawPacket->m_shaderResourceGroups = shaderResourceGroups;
            drawPacket->m_shaderResourceGroupCount = aznumeric_caster(m_shaderResourceGroups.size());
        }

        if (uniqueShaderResourceGroupsOffset.IsValid())
        {
            auto shaderResourceGroups = reinterpret_cast<const SingleDeviceShaderResourceGroup**>(allocationData + uniqueShaderResourceGroupsOffset.m_ptr);
            for (size_t i = 0; i < m_drawRequests.size(); ++i)
            {
                shaderResourceGroups[i] = m_drawRequests[i].m_uniqueShaderResourceGroup;
            }

            drawPacket->m_uniqueShaderResourceGroups = shaderResourceGroups;
            drawPacket->m_uniqueShaderResourceGroupCount = aznumeric_caster(m_drawRequests.size());
                
        }

        if (rootConstantsOffset.IsValid())
        {
            auto rootConstants = reinterpret_cast<uint8_t*>(allocationData + rootConstantsOffset.m_ptr);
            ::memcpy(rootConstants, m_rootConstants.data(), m_rootConstants.size() * sizeof(uint8_t));
            drawPacket->m_rootConstants = rootConstants;
            drawPacket->m_rootConstantSize = aznumeric_caster(m_rootConstants.size());
        }

        if (scissorOffset.IsValid())
        {
            auto scissors = reinterpret_cast<Scissor*>(allocationData + scissorOffset.m_ptr);
            ::memcpy(scissors, m_scissors.data(), m_scissors.size() * sizeof(Scissor));
            drawPacket->m_scissors = scissors;
            drawPacket->m_scissorsCount = aznumeric_caster(m_scissors.size());
        }

        if (viewportOffset.IsValid())
        {
            auto viewports = reinterpret_cast<Viewport*>(allocationData + viewportOffset.m_ptr);
            ::memcpy(viewports, m_viewports.data(), m_viewports.size() * sizeof(Viewport));
            drawPacket->m_viewports = viewports;
            drawPacket->m_viewportsCount = aznumeric_caster(m_viewports.size());
        }

        auto drawItems = reinterpret_cast<SingleDeviceDrawItem*>(allocationData + drawItemsOffset.m_ptr);
        auto drawItemSortKeys = reinterpret_cast<DrawItemSortKey*>(allocationData + drawItemSortKeysOffset.m_ptr);
        auto drawListTags = reinterpret_cast<DrawListTag*>(allocationData + drawListTagsOffset.m_ptr);
        auto drawFilterMasks = reinterpret_cast<DrawFilterMask*>(allocationData + drawFilterMasksOffset.m_ptr);
        drawPacket->m_drawItemCount = aznumeric_caster(m_drawRequests.size());
        drawPacket->m_drawItems = drawItems;
        drawPacket->m_drawItemSortKeys = drawItemSortKeys;
        drawPacket->m_drawListTags = drawListTags;
        drawPacket->m_drawFilterMasks = drawFilterMasks;

        const AZStd::vector<DrawListTag>& disabledTags = RHISystemInterface::Get()->GetDrawListTagsDisabledByDefault();

        for (size_t i = 0; i < m_drawRequests.size(); ++i)
        {
            const SingleDeviceDrawRequest& drawRequest = m_drawRequests[i];

            drawListTags[i] = drawRequest.m_listTag;
            drawFilterMasks[i] = drawRequest.m_drawFilterMask;
            drawItemSortKeys[i] = drawRequest.m_sortKey;

            bool drawListTagDisabled = false;
            for (const DrawListTag& disabledTag : disabledTags)
            {
                drawListTagDisabled = drawListTagDisabled || (drawRequest.m_listTag == disabledTag);
            }

            SingleDeviceDrawItem& drawItem = drawItems[i];
            drawItem.m_enabled = !drawListTagDisabled;
            drawItem.m_arguments = m_drawArguments;
            drawItem.m_stencilRef = drawRequest.m_stencilRef;
            drawItem.m_streamBufferViewCount = 0;
            drawItem.m_shaderResourceGroupCount = drawPacket->m_shaderResourceGroupCount;
            drawItem.m_rootConstantSize = drawPacket->m_rootConstantSize;
            drawItem.m_scissorsCount = drawPacket->m_scissorsCount;
            drawItem.m_viewportsCount = drawPacket->m_viewportsCount;
            drawItem.m_pipelineState = drawRequest.m_pipelineState;
            drawItem.m_indexBufferView = &drawPacket->m_indexBufferView;
            drawItem.m_streamBufferViews = nullptr;
            drawItem.m_rootConstants = drawPacket->m_rootConstants;
            drawItem.m_shaderResourceGroups = drawPacket->m_shaderResourceGroups;
            drawItem.m_uniqueShaderResourceGroup = drawPacket->m_uniqueShaderResourceGroups[i];
            drawItem.m_scissors = drawPacket->m_scissors;
            drawItem.m_viewports = drawPacket->m_viewports;
        }

        if (streamBufferViewsOffset.IsValid())
        {
            auto streamBufferViews = reinterpret_cast<SingleDeviceStreamBufferView*>(allocationData + streamBufferViewsOffset.m_ptr);

            drawPacket->m_streamBufferViews = streamBufferViews;
            drawPacket->m_streamBufferViewCount = aznumeric_caster(m_streamBufferViewCount);

            for (size_t i = 0; i < m_drawRequests.size(); ++i)
            {
                const SingleDeviceDrawRequest& drawRequest = m_drawRequests[i];

                if (!drawRequest.m_streamBufferViews.empty())
                {
                    drawItems[i].m_streamBufferViews = streamBufferViews;
                    drawItems[i].m_streamBufferViewCount = aznumeric_caster(drawRequest.m_streamBufferViews.size());

                    for (const SingleDeviceStreamBufferView& streamBufferView : drawRequest.m_streamBufferViews)
                    {
                        *streamBufferViews++ = streamBufferView;
                    }
                }
            }
        }

        ClearData();

        return drawPacket;
    }

    void SingleDeviceDrawPacketBuilder::ClearData()
    {
        m_allocator = nullptr;
        m_drawArguments = {};
        m_drawListMask.reset();
        m_streamBufferViewCount = 0;
        m_drawRequests.clear();
        m_shaderResourceGroups.clear();
        m_rootConstants = {};
        m_scissors.clear();
        m_viewports.clear();
    }

    SingleDeviceDrawPacket* SingleDeviceDrawPacketBuilder::Clone(const SingleDeviceDrawPacket* original)
    {
        Begin(original->m_allocator);
        SetDrawArguments(original->GetDrawItemProperties(0).m_item->m_arguments);
        SetIndexBufferView(original->m_indexBufferView);
        SetRootConstants(AZStd::span<const uint8_t>(original->m_rootConstants, original->m_rootConstantSize));
        SetScissors(AZStd::span<const Scissor>(original->m_scissors, original->m_scissorsCount));
        SetViewports(AZStd::span<const Viewport>(original->m_viewports, original->m_viewportsCount));
        for (uint8_t i = 0; i < original->m_shaderResourceGroupCount; ++i)
        {
            const SingleDeviceShaderResourceGroup* const* srg = original->m_shaderResourceGroups + i;
            AddShaderResourceGroup(*srg);
        }
        for (uint8_t i = 0; i < original->m_drawItemCount; ++i)
        {
            const SingleDeviceDrawItem* drawItem = original->m_drawItems + i;
            SingleDeviceDrawRequest drawRequest;
            drawRequest.m_drawFilterMask = *(original->m_drawFilterMasks + i);
            drawRequest.m_listTag = *(original->m_drawListTags + i);
            drawRequest.m_pipelineState = drawItem->m_pipelineState;
            drawRequest.m_sortKey = *(original->m_drawItemSortKeys + i);
            drawRequest.m_stencilRef = drawItem->m_stencilRef;
            drawRequest.m_streamBufferViews = AZStd::span(drawItem->m_streamBufferViews, drawItem->m_streamBufferViewCount);
            drawRequest.m_uniqueShaderResourceGroup = drawItem->m_uniqueShaderResourceGroup;
            AddDrawItem(drawRequest);
        }
        return End();
    }
}

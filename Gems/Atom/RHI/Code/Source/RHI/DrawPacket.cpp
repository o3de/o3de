/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/ConstantsData.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <AzCore/Memory/Memory.h>
#include <cstdint>
namespace AZ
{
    namespace RHI
    {
        size_t DrawPacket::GetDrawItemCount() const
        {
            return m_drawItemCount;
        }

        DrawItemProperties DrawPacket::GetDrawItem(size_t index) const
        {
            AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
            return DrawItemProperties(&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMasks[index]);
        }

        DrawListTag DrawPacket::GetDrawListTag(size_t index) const
        {
            AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
            return m_drawListTags[index];
        }

        DrawFilterMask DrawPacket::GetDrawFilterMask(size_t index) const
        {
            AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
            return m_drawFilterMasks[index];
        }

        DrawListMask DrawPacket::GetDrawListMask() const
        {
            return m_drawListMask;
        }

        void DrawPacket::operator delete(void* p, [[maybe_unused]] size_t size)
        {
            reinterpret_cast<const DrawPacket*>(p)->m_allocator->DeAllocate(p);
        }

        DrawPacket* DrawPacket::Clone(
            /* [[maybe_unused]] const DrawArguments& drawArguments, [[maybe_unused]] const AZ::RHI::ConstantsData& rootConstants*/) const
        {
            uint8_t* allocationData = reinterpret_cast<uint8_t*>(m_allocator->Allocate(m_allocationSize, AZStd::alignment_of<DrawPacket>::value));
            DrawPacket* clone = new (allocationData) DrawPacket();

            // Anything that is pure data can be copied directly, but we can't just copy the whole object because then we'd also be copying
            // the intrusive_ptr refcount, which we don't want. So instead, we need to copy everything individually
            clone->m_allocator = m_allocator;
            clone->m_allocationSize = m_allocationSize;
            clone->m_drawListMask = m_drawListMask;
            clone->m_indexBufferView = m_indexBufferView;
            clone->m_drawItemCount = m_drawItemCount;
            clone->m_streamBufferViewCount = m_streamBufferViewCount;
            clone->m_shaderResourceGroupCount = m_shaderResourceGroupCount;
            clone->m_uniqueShaderResourceGroupCount = m_uniqueShaderResourceGroupCount;
            clone->m_rootConstantSize = m_rootConstantSize;
            clone->m_scissorsCount = m_scissorsCount;
            clone->m_viewportsCount = m_viewportsCount;

            // The member variable pointers in a DrawPacket refer to different offsets in the contiguous set of allocated data
            // The offset from the start of this object to a pointer should be the same as
            // the offset from the start of the clone to the start of the cloned member pointer
            intptr_t cloneStart = reinterpret_cast<intptr_t>(clone);
            intptr_t originalStart = reinterpret_cast<intptr_t>(this);

            // Anything that is a pointer should match 1:1 the offset found in the original drawpacket
            // Then the actual data should be copied
            clone->m_drawItems = (DrawItem*)(cloneStart + ((intptr_t)m_drawItems - originalStart));
            memcpy((void*)clone->m_drawItems, m_drawItems, m_drawItemCount * sizeof(DrawItem));

            clone->m_drawItemSortKeys = (DrawItemSortKey*)(cloneStart + ((intptr_t)m_drawItemSortKeys - originalStart));
            memcpy((void*)clone->m_drawItemSortKeys, m_drawItemSortKeys, m_drawItemCount * sizeof(DrawItemSortKey));

            clone->m_drawListTags = (DrawListTag*)(cloneStart + ((intptr_t)m_drawListTags - originalStart));
            memcpy((void*)clone->m_drawListTags, m_drawListTags, m_drawItemCount * sizeof(DrawListTag));

            clone->m_drawFilterMasks = (DrawFilterMask*)(cloneStart + ((intptr_t)m_drawFilterMasks - originalStart));
            memcpy((void*)clone->m_drawFilterMasks, m_drawFilterMasks, m_drawItemCount * sizeof(DrawFilterMask));

            // TODO: These two are dubious, pointers to pointers, need to double check where things are pointing to
            // and make sure we don't need to do a deep copy of the shader resource groups.
            clone->m_shaderResourceGroups = (ShaderResourceGroup**)(cloneStart + ((intptr_t)m_shaderResourceGroups - originalStart));
            memcpy((void*)clone->m_shaderResourceGroups, m_shaderResourceGroups, m_shaderResourceGroupCount * sizeof(ShaderResourceGroup*));

            clone->m_uniqueShaderResourceGroups =
                (ShaderResourceGroup**)(cloneStart + ((intptr_t)m_uniqueShaderResourceGroups - originalStart));
            memcpy(
                (void*)clone->m_uniqueShaderResourceGroups,
                m_uniqueShaderResourceGroups,
                m_uniqueShaderResourceGroupCount * sizeof(ShaderResourceGroup*));


            clone->m_rootConstants = (uint8_t*)(cloneStart + ((intptr_t)m_rootConstants - originalStart));
            memcpy((void*)clone->m_rootConstants, m_rootConstants, m_rootConstantSize * sizeof(uint8_t));

            clone->m_streamBufferViews = (StreamBufferView*)(cloneStart + ((intptr_t)m_streamBufferViews - originalStart));
            memcpy((void*)clone->m_streamBufferViews, m_streamBufferViews, m_streamBufferViewCount * sizeof(StreamBufferView));

            clone->m_scissors = (Scissor*)(cloneStart + ((intptr_t)m_scissors - originalStart));
            memcpy((void*)clone->m_scissors, m_scissors, m_scissorsCount * sizeof(Scissor));

            clone->m_viewports = (Viewport*)(cloneStart + ((intptr_t)m_viewports - originalStart));
            memcpy((void*)clone->m_viewports, m_viewports, m_viewportsCount * sizeof(Viewport));

            return clone;
        }

        
        void DrawPacket::SetRootConstant(size_t drawItemIndex, const Interval& interval, const AZStd::span<uint8_t>& data)
        {
            const DrawItem* drawItem = m_drawItems + drawItemIndex;
            AZ_Assert(
                drawItem->m_rootConstantSize > interval.m_min + data.size(),
                "Attempting to set root constants that don't match the size of the DrawItem's root constant size.");
            memcpy(
                (void*)(drawItem->m_rootConstants + interval.m_min),
                data.data(),
                data.size());
        }
    }
}

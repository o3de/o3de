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

namespace AZ::RHI
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

    void DrawPacket::SetRootConstant(uint32_t offset, const AZStd::span<uint8_t>& data)
    {
        bool sizeValid = data.size() <= aznumeric_cast<uint32_t>(m_rootConstantSize) - offset;
        if (sizeValid)
        {
            memcpy((void*)(m_rootConstants + offset), data.data(), data.size());
        }
        else
        {
            AZ_Assert(sizeValid, "New root constants exceed the original size.");
        }
    }

    void DrawPacket::SetInstanceCount(uint32_t instanceCount)
    {
        for (size_t drawItemIndex = 0; drawItemIndex < m_drawItemCount; ++drawItemIndex)
        {
            const DrawItem* drawItemConst = m_drawItems + drawItemIndex;
            // Need to mutate for mesh instancing.
            // This should be used after cloning the draw packet from DrawPacketBuilder.
            DrawItem* drawItem = const_cast<DrawItem*>(drawItemConst);
            drawItem->m_arguments.m_indexed.m_instanceCount = instanceCount;
        }
    }
}

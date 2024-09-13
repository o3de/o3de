/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/ConstantsData.h>
#include <Atom/RHI/DeviceDrawPacket.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <AzCore/Memory/Memory.h>

namespace AZ::RHI
{
    size_t DeviceDrawPacket::GetDrawItemCount() const
    {
        return m_drawItemCount;
    }

    s32 DeviceDrawPacket::GetDrawListIndex(DrawListTag drawListTag) const
    {
        for (size_t i = 0; i < m_drawItemCount; ++i)
        {
            if (GetDrawListTag(i) == drawListTag)
            {
                return s32(i);
            }
        }
        return -1;
    }

    DeviceDrawItem* DeviceDrawPacket::GetDrawItem(size_t index)
    {
        return (index < m_drawItemCount) ? &m_drawItems[index] : nullptr;
    }

    DeviceDrawItem* DeviceDrawPacket::GetDrawItem(DrawListTag drawListTag)
    {
        s32 index = GetDrawListIndex(drawListTag);
        if (index > -1)
        {
            return GetDrawItem(index);
        }
        return nullptr;
    }

    DeviceDrawItemProperties DeviceDrawPacket::GetDrawItemProperties(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return DeviceDrawItemProperties{&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMasks[index]};
    }

    DrawListTag DeviceDrawPacket::GetDrawListTag(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return m_drawListTags[index];
    }

    DrawFilterMask DeviceDrawPacket::GetDrawFilterMask(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return m_drawFilterMasks[index];
    }

    DrawListMask DeviceDrawPacket::GetDrawListMask() const
    {
        return m_drawListMask;
    }

    void DeviceDrawPacket::operator delete(void* p, [[maybe_unused]] size_t size)
    {
        reinterpret_cast<const DeviceDrawPacket*>(p)->m_allocator->DeAllocate(p);
    }

    void DeviceDrawPacket::SetRootConstant(uint32_t offset, const AZStd::span<uint8_t>& data)
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

    void DeviceDrawPacket::SetInstanceCount(uint32_t instanceCount)
    {
        for (size_t drawItemIndex = 0; drawItemIndex < m_drawItemCount; ++drawItemIndex)
        {
            const DeviceDrawItem* drawItemConst = m_drawItems + drawItemIndex;
            // Need to mutate for mesh instancing.
            // This should be used after cloning the draw packet from DeviceDrawPacketBuilder.
            DeviceDrawItem* drawItem = const_cast<DeviceDrawItem*>(drawItemConst);
            drawItem->m_drawInstanceArgs.m_instanceCount = instanceCount;
        }
    }
}

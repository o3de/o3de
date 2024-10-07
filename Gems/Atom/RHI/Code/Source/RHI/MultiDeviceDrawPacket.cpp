/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI/ConstantsData.h>
#include <Atom/RHI/MultiDeviceDrawPacket.h>

#include <AzCore/Memory/Memory.h>

namespace AZ::RHI
{
    size_t MultiDeviceDrawPacket::GetDrawItemCount() const
    {
        return m_drawItems.size();
    }

    s32 MultiDeviceDrawPacket::GetDrawListIndex(DrawListTag drawListTag) const
    {
        for (size_t i = 0; i < m_drawItems.size(); ++i)
        {
            if (GetDrawListTag(i) == drawListTag)
            {
                return s32(i);
            }
        }
        return -1;
    }

    MultiDeviceDrawItem* MultiDeviceDrawPacket::GetDrawItem(size_t index)
    {
        return (index < m_drawItems.size()) ? &m_drawItems[index] : nullptr;
    }

    MultiDeviceDrawItem* MultiDeviceDrawPacket::GetDrawItem(DrawListTag drawListTag)
    {
        s32 index = GetDrawListIndex(drawListTag);
        if (index > -1)
        {
            return GetDrawItem(index);
        }
        return nullptr;
    }

    MultiDeviceDrawItemProperties MultiDeviceDrawPacket::GetDrawItemProperties(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return MultiDeviceDrawItemProperties{&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMasks[index]};
    }

    DrawListTag MultiDeviceDrawPacket::GetDrawListTag(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return m_drawListTags[index];
    }

    DrawFilterMask MultiDeviceDrawPacket::GetDrawFilterMask(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return m_drawFilterMasks[index];
    }

    DrawListMask MultiDeviceDrawPacket::GetDrawListMask() const
    {
        return m_drawListMask;
    }

    void MultiDeviceDrawPacket::SetRootConstant(uint32_t offset, const AZStd::span<uint8_t>& data)
    {
        for(auto&[_, deviceDrawPacket] : m_deviceDrawPackets)
        {
            deviceDrawPacket->SetRootConstant(offset, data);
        }
    }

    void MultiDeviceDrawPacket::SetInstanceCount(uint32_t instanceCount)
    {
        for (auto& drawItem : m_drawItems)
        {
            drawItem.SetIndexedArgumentsInstanceCount(instanceCount);
        }
    }
} // namespace AZ::RHI

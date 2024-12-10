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
#include <Atom/RHI/DrawPacket.h>

#include <AzCore/Memory/Memory.h>

namespace AZ::RHI
{
    size_t DrawPacket::GetDrawItemCount() const
    {
        return m_drawItems.size();
    }

    s32 DrawPacket::GetDrawListIndex(DrawListTag drawListTag) const
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

    DrawItem* DrawPacket::GetDrawItem(size_t index)
    {
        return (index < m_drawItems.size()) ? &m_drawItems[index] : nullptr;
    }

    const DrawItem* DrawPacket::GetDrawItem(size_t index) const
    {
        return (index < m_drawItems.size()) ? &m_drawItems[index] : nullptr;
    }

    DrawItem* DrawPacket::GetDrawItem(DrawListTag drawListTag)
    {
        s32 index = GetDrawListIndex(drawListTag);
        if (index > -1)
        {
            return GetDrawItem(index);
        }
        return nullptr;
    }

    const DrawItem* DrawPacket::GetDrawItem(DrawListTag drawListTag) const
    {
        s32 index = GetDrawListIndex(drawListTag);
        if (index > -1)
        {
            return GetDrawItem(index);
        }
        return nullptr;
    }

    DrawItemProperties DrawPacket::GetDrawItemProperties(size_t index) const
    {
        AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
        return DrawItemProperties{&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMasks[index]};
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

    void DrawPacket::SetRootConstant(uint32_t offset, const AZStd::span<uint8_t>& data)
    {
        for(auto&[_, deviceDrawPacket] : m_deviceDrawPackets)
        {
            deviceDrawPacket->SetRootConstant(offset, data);
        }
    }

    void DrawPacket::SetInstanceCount(uint32_t instanceCount)
    {
        for (auto& [_, deviceDrawPacket] : m_deviceDrawPackets)
        {
            deviceDrawPacket->SetInstanceCount(instanceCount);
        }
    }
} // namespace AZ::RHI

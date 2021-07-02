/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RHI/DrawPacket.h>

#include <AzCore/Memory/Memory.h>

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
            return DrawItemProperties(&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMask);
        }

        DrawListTag DrawPacket::GetDrawListTag(size_t index) const
        {
            AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
            return m_drawListTags[index];
        }

        DrawFilterMask DrawPacket::GetDrawFilterMask() const
        {
            return m_drawFilterMask;
        }

        DrawListMask DrawPacket::GetDrawListMask() const
        {
            return m_drawListMask;
        }

        void DrawPacket::operator delete(void* p, [[maybe_unused]] size_t size)
        {
            reinterpret_cast<const DrawPacket*>(p)->m_allocator->DeAllocate(p);
        }
    }
}

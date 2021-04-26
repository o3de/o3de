/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        DrawItemKeyPair DrawPacket::GetDrawItem(size_t index) const
        {
            AZ_Assert(index < GetDrawItemCount(), "Out of bounds array access!");
            return DrawItemKeyPair(&m_drawItems[index], m_drawItemSortKeys[index], m_drawFilterMask);
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

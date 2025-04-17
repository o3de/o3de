/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DrawList.h>

#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    DrawListView GetDrawListPartition(DrawListView drawList, size_t partitionIndex, size_t partitionCount)
    {
        if (drawList.empty())
        {
            return DrawListView{};
        }

        const size_t itemsPerPartition = AZ::DivideAndRoundUp(drawList.size(), partitionCount);
        const size_t itemOffset = partitionIndex * itemsPerPartition;
        const size_t itemCount = AZStd::min(drawList.size() - itemOffset, itemsPerPartition);
        return DrawListView(&drawList[itemOffset], itemCount);
    }

    void SortDrawList(DrawList& drawList, DrawListSortType sortType)
    {
        switch (sortType)
        {
        case DrawListSortType::KeyThenDepth:
            AZStd::sort(drawList.begin(), drawList.end(), [](const DrawItemProperties& a, const DrawItemProperties& b)
                {
                    if (a.m_sortKey != b.m_sortKey)
                    {
                        return a.m_sortKey < b.m_sortKey;
                    }
                    if (a.m_depth != b.m_depth)
                    {
                        return a.m_depth < b.m_depth;
                    }
                    return a.m_item < b.m_item;
                }
            );
            break;

        case DrawListSortType::KeyThenReverseDepth:
            AZStd::sort(drawList.begin(), drawList.end(), [](const DrawItemProperties& a, const DrawItemProperties& b)
                {
                    if (a.m_sortKey != b.m_sortKey)
                    {
                        return a.m_sortKey < b.m_sortKey;
                    }
                    if (a.m_depth != b.m_depth)
                    {
                        return a.m_depth > b.m_depth;
                    }
                    return a.m_item < b.m_item;
                }
            );
            break;

        case DrawListSortType::DepthThenKey:
            AZStd::sort(drawList.begin(), drawList.end(), [](const DrawItemProperties& a, const DrawItemProperties& b)
                {
                    if (a.m_depth != b.m_depth)
                    {
                        return a.m_depth < b.m_depth;
                    }
                    if (a.m_sortKey != b.m_sortKey)
                    {
                        return a.m_sortKey < b.m_sortKey;
                    }
                    return a.m_item < b.m_item;
                }
            );
            break;

        case DrawListSortType::ReverseDepthThenKey:
            AZStd::sort(drawList.begin(), drawList.end(), [](const DrawItemProperties& a, const DrawItemProperties& b)
                {
                    if (a.m_depth != b.m_depth)
                    {
                        return a.m_depth > b.m_depth;
                    }
                    if (a.m_sortKey != b.m_sortKey)
                    {
                        return a.m_sortKey < b.m_sortKey;
                    }
                    return a.m_item < b.m_item;
                }
            );
            break;
        }
    }
}

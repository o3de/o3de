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
#include <Atom/RHI/DrawList.h>

#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        DrawListView GetDrawListPartition(DrawListView drawList, size_t partitionIndex, size_t partitionCount)
        {
            if (drawList.empty())
            {
                return DrawListView{};
            }

            const size_t itemsPerPartition = DivideByMultiple(drawList.size(), partitionCount);
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
                        return a.m_depth < b.m_depth;
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
                        return a.m_depth > b.m_depth;
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
                        return a.m_sortKey < b.m_sortKey;
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
                        return a.m_sortKey < b.m_sortKey;
                    }
                );
                break;
            }
        }
    }
}

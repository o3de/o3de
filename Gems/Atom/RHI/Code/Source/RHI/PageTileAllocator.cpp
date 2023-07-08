/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/PageTileAllocator.h>

#include <AzCore/Casting/numeric_cast.h>    // for aznumeric_cast

namespace AZ::RHI
{
    void PageTileAllocator::Init(uint32_t totalTileCount)
    {
        m_totalTileCount = totalTileCount;
        m_allocatedTileCount = 0;
        m_freeList.push_back(PageTileSpan(0, totalTileCount));
    }

    AZStd::vector<PageTileSpan> PageTileAllocator::TryAllocate(uint32_t tileCountRequested, uint32_t& tileCountAllocated)
    {
        tileCountAllocated = 0;
        AZStd::vector<PageTileSpan> allocatedTiles;

        // Allocate desired amount of tiles or until the free list is empty
        while (tileCountAllocated < tileCountRequested && m_freeList.size())
        {
            uint32_t tileNeeded = tileCountRequested - tileCountAllocated;
            PageTileSpan tiles = m_freeList.back();
                
            if (tiles.m_tileCount <= tileNeeded)
            {
                // if the current node has tiles same or less than needed
                // remove the node from the free list
                m_freeList.pop_back();
            }
            else
            {
                // the current node has more tiles than needed.
                // split existing one. Keep the first part in the free list and use the second part for allocation 
                tiles.m_offset = tiles.m_offset + tiles.m_tileCount - tileNeeded;
                tiles.m_tileCount = tileNeeded;
                m_freeList.back().m_tileCount -= tileNeeded;
            }

            tileCountAllocated += tiles.m_tileCount;
            allocatedTiles.push_back(tiles);
        }

        m_allocatedTileCount += tileCountAllocated; 
        AZ_Assert(m_allocatedTileCount <= m_totalTileCount, "Implementation error: tile count error.");

        return allocatedTiles;
    }

    void PageTileAllocator::DeAllocate(const AZStd::vector<PageTileSpan>& tilesList)
    {
        for (auto tiles : tilesList)
        {
            DeAllocate(tiles);
        }
    }

    void PageTileAllocator::DeAllocate(PageTileSpan tiles)
    {
        // update tile count before the tiles get changed
        m_allocatedTileCount -= tiles.m_tileCount;

        auto itr = AZStd::upper_bound(AZStd::begin(m_freeList), AZStd::end(m_freeList), tiles, PageTileSpan::Compare());

        int64_t pos = itr - m_freeList.begin();

        if (pos > 0)
        {
            // check if the tile can merge with the element in front of 
            PageTileSpan before = m_freeList.at(pos - 1);
            if (before.m_offset + before.m_tileCount == tiles.m_offset)
            {
                tiles.m_offset = before.m_offset;
                tiles.m_tileCount += before.m_tileCount;
                // remove before 
                m_freeList.erase(m_freeList.begin()+pos-1);
                // update insert position 
                pos -= 1;
            }
        }

        if (pos < aznumeric_cast<int64_t>(m_freeList.size()))
        {
            // check if the tile can merge with the following element
            PageTileSpan after = m_freeList.at(pos);
            if (tiles.m_offset + tiles.m_tileCount == after.m_offset)
            {
                tiles.m_tileCount += after.m_tileCount;
                // remove the following element
                m_freeList.erase(m_freeList.begin()+pos);
            }
        }

        m_freeList.insert(m_freeList.begin()+pos, tiles);
    }

    uint32_t PageTileAllocator::GetFreeTileCount() const
    {
        return m_totalTileCount - m_allocatedTileCount;
    }

    uint32_t PageTileAllocator::GetUsedTileCount() const
    {
        return m_allocatedTileCount;
    }

    uint32_t PageTileAllocator::GetTotalTileCount() const
    {
        return m_totalTileCount;
    }

    bool PageTileAllocator::IsPageFree() const
    {
        return m_allocatedTileCount == 0;
    }

    const AZStd::vector<PageTileSpan>& PageTileAllocator::GetFreeList() const
    {
        return m_freeList;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ::RHI
{
    //! a structure to represent continous number of tiles 
    struct PageTileSpan
    {
    public:
        struct Compare {
            bool operator()(const PageTileSpan& a, const PageTileSpan& b) const {
                return a.m_offset < b.m_offset;
            }
        };

        PageTileSpan(uint32_t offset, uint32_t count)
            : m_offset(offset)
            , m_tileCount(count)
        {
        }
        // offset by tile
        uint32_t m_offset;
        // tile count
        uint32_t m_tileCount;

    };

    //! This allocator allocates tile groups from a page which is aligned by tiles.
    class PageTileAllocator
    {
    public:
        void Init(uint32_t totalTileCount);

        //! Allocate tiles. It returns tiles which are avaliable.
        //! It may return tiles which less than desired count
        //! @param tileCount Desired tile count to be allocated
        //! @param allocatedTileCount Actual tile count was allocated
        AZStd::vector<PageTileSpan> TryAllocate(uint32_t tileCount, /*out*/ uint32_t& allocatedTileCount);

        //! DeAllocate multiple group of tiles 
        void DeAllocate(const AZStd::vector<PageTileSpan>& tiles);

        //! DeAllocate one group of tiles. If the tiles are adjacented to the tiles in the freelist, they would be merged
        void DeAllocate(PageTileSpan tiles);

        uint32_t GetFreeTileCount() const;
        uint32_t GetUsedTileCount() const;
        uint32_t GetTotalTileCount() const;

        //! Returns whether all tiles in this page are avaliable
        bool IsPageFree() const;

        //! Get tile groups in free list
        const AZStd::vector<PageTileSpan>& GetFreeList() const;

    private:
        uint32_t m_allocatedTileCount = 0;
        uint32_t m_totalTileCount = 0;

        //list of free tile groups; tile groups are in ascending order based on their offsets
        AZStd::vector<PageTileSpan> m_freeList;
    };
}

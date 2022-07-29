/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <AzCore/UnitTest/UnitTest.h>
#include <Atom/RHI/PageTileAllocator.h>
#include <AzCore/Math/Random.h>

namespace UnitTest
{
    using namespace AZ;

    class PageTileAllocatorTest
        : public RHITestFixture
    {
    public:
        PageTileAllocatorTest()
            : RHITestFixture()
        {}

        // Return ture if there is no overlapping between all the input tile groups
        bool ValidateTilesNotOverlap(const AZStd::vector<RHI::Tiles>& tilesList)
        {
            AZStd::set<RHI::Tiles, RHI::Tiles::Compare> sortedTilesList;

            for (const auto& tiles : tilesList)
            {
                sortedTilesList.insert(tiles);
            }

            uint32_t lastTile = 0;
            for (const auto& tiles1 : sortedTilesList)
            {
                uint32_t newlastTile = tiles1.m_offset + tiles1.m_tileCount;
                if (lastTile < newlastTile)
                {
                    lastTile = newlastTile;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        uint32_t GetTileCount(const AZStd::vector<RHI::Tiles>& tilesList)
        {
            uint32_t tileCount = 0;
            for (const auto& tiles : tilesList)
            {
                tileCount += tiles.m_tileCount;
            }
            return tileCount;
        }
    };

    TEST_F(PageTileAllocatorTest, SingleAllocation_Success)
    {
        RHI::PageTileAllocator allocator;
        
        uint32_t pageTileCount = 256;
        allocator.Init(pageTileCount);

        ASSERT_TRUE(allocator.GetFreeTileCount() == pageTileCount);
        ASSERT_TRUE(allocator.GetTotalTileCount() == pageTileCount);
        ASSERT_TRUE(allocator.GetUsedTileCount() == 0);

        uint32_t allocated = 0;
        uint32_t requested = 24;

        AZStd::vector<RHI::Tiles> tilesList = allocator.TryAllocate(requested, allocated);
        ASSERT_TRUE(allocated == requested);
        ASSERT_TRUE(tilesList.size() == 1);
        ASSERT_TRUE(tilesList.front().m_tileCount == requested);
        ASSERT_FALSE(allocator.IsPageFree());
        
        ASSERT_TRUE(allocator.GetFreeTileCount() == pageTileCount - requested);
        ASSERT_TRUE(allocator.GetUsedTileCount() == requested);

        allocator.DeAllocate(tilesList);
        
        ASSERT_TRUE(allocator.GetFreeTileCount() == pageTileCount);
        ASSERT_TRUE(allocator.GetUsedTileCount() == 0);
        ASSERT_TRUE(allocator.IsPageFree());
    }

    TEST_F(PageTileAllocatorTest, SingleOutOfRangeAllocation_Failed)
    {
        RHI::PageTileAllocator allocator;
        
        uint32_t pageTileCount = 20;
        allocator.Init(pageTileCount);

        uint32_t allocated = 0;
        uint32_t requested = 24;

        AZStd::vector<RHI::Tiles> tilesList = allocator.TryAllocate(requested, allocated);

        ASSERT_TRUE(allocated == pageTileCount);
        ASSERT_TRUE(tilesList.front().m_tileCount == allocated);
        ASSERT_FALSE(allocator.IsPageFree());
        
        ASSERT_TRUE(allocator.GetFreeTileCount() == 0);
        ASSERT_TRUE(allocator.GetUsedTileCount() == pageTileCount);

        allocator.DeAllocate(tilesList);
        
        ASSERT_TRUE(allocator.GetFreeTileCount() == pageTileCount);
        ASSERT_TRUE(allocator.GetUsedTileCount() == 0);
        ASSERT_TRUE(allocator.IsPageFree());
    }

    TEST_F(PageTileAllocatorTest, RandomAllocationDeallocation_Success)
    {
        RHI::PageTileAllocator allocator;
        
        uint32_t pageTileCount = 30;
        allocator.Init(pageTileCount);

        uint32_t allocationCount = 100;

        AZ::SimpleLcgRandom random(AZStd::GetTimeNowMicroSecond());

        AZStd::vector<RHI::Tiles> allocatedTilesList;

        while (allocationCount-- != 0)
        {
            // 51% chance of doing an add. Biased towards adds so we fill up the allocator.
            if ((random.GetRandom() % 100) <= 51 || allocatedTilesList.size() == 0)
            {
                uint32_t requested = random.GetRandom() % (pageTileCount + 10);
                uint32_t allocated = 0;
                AZStd::vector<RHI::Tiles> tilesList = allocator.TryAllocate(requested, allocated);
                allocatedTilesList.insert(AZStd::end(allocatedTilesList), AZStd::begin(tilesList), AZStd::end(tilesList));

                ASSERT_TRUE(allocated <= requested);
                ASSERT_TRUE(ValidateTilesNotOverlap(allocatedTilesList));
                ASSERT_TRUE(GetTileCount(allocatedTilesList) == allocator.GetUsedTileCount());
            }
            else
            {
                // select some tile groups from the list
                uint32_t count = random.GetRandom() % allocatedTilesList.size() + 1;
                AZStd::vector<RHI::Tiles> tilesToBeRemoved;
                for (uint32_t i = 0; i < count; i++)
                {
                    uint32_t position = (random.GetRandom() % allocatedTilesList.size());
                    tilesToBeRemoved.push_back(allocatedTilesList[position]);
                    allocatedTilesList.erase(allocatedTilesList.begin() +position);
                }
                allocator.DeAllocate(tilesToBeRemoved);

                ASSERT_TRUE(ValidateTilesNotOverlap(allocator.GetFreeList()));
                ASSERT_TRUE(GetTileCount(allocatedTilesList) == allocator.GetUsedTileCount());
            }
        }
        
        allocator.DeAllocate(allocatedTilesList);

        ASSERT_TRUE(allocator.GetFreeList().size() == 1);
        ASSERT_TRUE(allocator.GetFreeTileCount() == pageTileCount);
        ASSERT_TRUE(allocator.GetUsedTileCount() == 0);
    }
}

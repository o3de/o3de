/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

namespace AZ::RHI
{
    namespace
    {
        const static bool TileAllocatorOutputDebugInfo = false;
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::Init(const Descriptor& descriptor, HeapAllocator& heapAllocator)
    {
        AZ_Assert(descriptor.m_heapMemoryUsage, "You must supply a valid pointer of HeapMemoryUsage.");

        m_descriptor = descriptor;
        m_heapAllocator = &heapAllocator;

        m_tileCountPerPage = aznumeric_cast<uint32_t>(m_heapAllocator->GetFactory().GetDescriptor().m_pageSizeInBytes)/descriptor.m_tileSizeInBytes;
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::AllocateFromFreeList(uint32_t tileCount, AZStd::vector<HeapTiles>& output)
    {
        uint32_t allocatedTileCount = 0;

        while ( !m_freeList.empty() && allocatedTileCount < tileCount)
        {
            uint32_t neededTileCount = tileCount - allocatedTileCount;
            auto freeListItr = m_freeList.begin();
            RHI::Ptr<Heap> heap = *freeListItr;
            auto itr = m_pageContexts.find(heap);

            if (itr != m_pageContexts.end())
            {
                uint32_t allocated = 0;
                auto tilesGroups = itr->second.TryAllocate(neededTileCount, allocated);

                if (itr->second.GetFreeTileCount() == 0)
                {
                    m_freeList.erase(freeListItr);
                }

                if (allocated == 0)
                {
                    AZ_Assert(false, "Implementation error: heap page in free list doesn't have tiles available");
                }

                // add tiles to result
                HeapTiles heapTiles;
                for (const RHI::PageTileSpan& tileSpan:tilesGroups)
                {
                    heapTiles.m_tileSpanList.push_back(tileSpan);
                    heapTiles.m_totalTileCount += tileSpan.m_tileCount;
                }
                heapTiles.m_heap = itr->first;
                output.emplace_back(heapTiles);
                allocatedTileCount += allocated;
            }
            else
            {
                AZ_Assert(false, "Implementation error: heap page context is missing.");
            }
        }
            
        AZ_Assert(allocatedTileCount == tileCount, "Implementation error: incomplete allocation");

        m_allocatedTileCount += tileCount;
        AZ_Assert(m_allocatedTileCount <= m_totalTileCount, "Implementation error: tile count error.");
            
        RHI::HeapMemoryUsage* heapMemoryUsage = m_descriptor.m_heapMemoryUsage;
        heapMemoryUsage->m_usedResidentInBytes += tileCount * m_descriptor.m_tileSizeInBytes;
    }
        
    template<typename Traits> 
    size_t TileAllocator<Traits>::EvaluateMemoryAllocation(uint32_t tileCount)
    {
        uint32_t freeTileCount = m_totalTileCount - m_allocatedTileCount;
        if (freeTileCount < tileCount)
        {
            uint32_t newPageCount = AZ::DivideAndRoundUp(tileCount - freeTileCount, m_tileCountPerPage);
            return newPageCount * m_heapAllocator->GetFactory().GetDescriptor().m_pageSizeInBytes;
        }
        return 0;
    }
        
    template<typename Traits> 
    AZStd::vector<typename TileAllocator<Traits>::HeapTiles> TileAllocator<Traits>::Allocate(uint32_t tileCount)
    {
        AZStd::vector<HeapTiles> tilesList;
            
        // Create new pages if there aren't enough free tiles available
        uint32_t freeTileCount = m_totalTileCount - m_allocatedTileCount;
        if (freeTileCount < tileCount)
        {
            uint32_t newPageCount = AZ::DivideAndRoundUp(tileCount - freeTileCount, m_tileCountPerPage);

            for (uint32_t pageIdx = 0; pageIdx < newPageCount; pageIdx++)
            {
                auto heap = m_heapAllocator->Allocate();
                if (!heap)
                {
                    // Return directly if we can't create more heaps
                    AZ_Warning("TileAllocator", false, "Failed to create a heap page");
                    return tilesList;
                }

                // setup page context for new heap page
                auto& pageTileAllocator = m_pageContexts[heap];
                pageTileAllocator.Init(m_tileCountPerPage);

                // add page to free list
                m_freeList.insert(heap);
                m_totalTileCount += m_tileCountPerPage;
            }
        }

        // allocate from free list
        AllocateFromFreeList(tileCount, tilesList);

        DebugPrintInfo("Allocate");

        return tilesList;
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::DeAllocate(const AZStd::vector<HeapTiles>& tilesGroups)
    {
        RHI::HeapMemoryUsage* heapMemoryUsage = m_descriptor.m_heapMemoryUsage;
        for (const auto& heapTiles : tilesGroups)
        {
            auto itr = m_pageContexts.find(heapTiles.m_heap);
            if (itr == m_pageContexts.end())
            {
                AZ_Assert(false, "Heap wasn't allocated by this allocator");
            }
            else
            {
                itr->second.DeAllocate(heapTiles.m_tileSpanList);
                AZ_Assert(itr->second.GetFreeTileCount() > 0, "De-allocate tiles from heap failed");
                m_freeList.insert(itr->first);

                m_allocatedTileCount -= heapTiles.m_totalTileCount;
                    
                heapMemoryUsage->m_usedResidentInBytes -= heapTiles.m_totalTileCount * m_descriptor.m_tileSizeInBytes;
            }
        }
            
        DebugPrintInfo("DeAllocate");
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::GarbageCollect()
    {
        if (!m_heapAllocator)
        {
            return;
        }

        auto itr = m_pageContexts.begin();
        while (itr != m_pageContexts.end())
        {
            if (itr->second.IsPageFree())
            {
                m_freeList.erase(itr->first);
                m_heapAllocator->DeAllocate(itr->first.get());
                itr = m_pageContexts.erase(itr);
                m_totalTileCount -= m_tileCountPerPage;
            }
            else
            {
                itr++;
            }
        }

        m_heapAllocator->Collect();

        DebugPrintInfo("GarbageCollect");
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::Shutdown()
    {
        GarbageCollect();

        AZ_Assert(m_allocatedTileCount == 0 && m_pageContexts.size() == 0 && m_freeList.size() == 0 && m_totalTileCount == 0,
            "Image resources which are using tiles are not released");
    }
        
    template<typename Traits> 
    uint32_t TileAllocator<Traits>::GetAllocatedTileCount() const
    {
        return m_allocatedTileCount;
    }
        
    template<typename Traits> 
    uint32_t TileAllocator<Traits>::GetTotalTileCount() const
    {
        return m_totalTileCount;
    }
        
    template<typename Traits> 
    const typename TileAllocator<Traits>::Descriptor& TileAllocator<Traits>::GetDescriptor() const
    {
        return m_descriptor;
    }
        
    template<typename Traits> 
    void TileAllocator<Traits>::DebugPrintInfo([[maybe_unused]] const char* opName) const
    {
        if (TileAllocatorOutputDebugInfo)
        {
            RHI::HeapMemoryUsage* heapMemoryUsage = m_descriptor.m_heapMemoryUsage;

            AZ_TracePrintf(
                "TileAllocator",
                "%p %s: tiles %d/%d resident memory %d/%d/%d\n",
                this,
                opName,
                m_allocatedTileCount,
                m_totalTileCount,
                heapMemoryUsage->m_usedResidentInBytes.load(),
                heapMemoryUsage->m_totalResidentInBytes.load(),
                heapMemoryUsage->m_budgetInBytes);

            if (m_allocatedTileCount != heapMemoryUsage->m_usedResidentInBytes.load() / m_descriptor.m_tileSizeInBytes)
            {
                AZ_Assert(false, "Memory usage data implementation error");
            }
        }
    }
}


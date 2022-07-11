/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/HeapAllocator.h>

namespace AZ
{
    namespace DX12
    {
        void HeapFactory::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_getHeapMemoryUsageFunction, "You must supply a valid function for getting heap memory usage.");

            m_descriptor = descriptor;
            m_descriptor.m_pageSizeInBytes = RHI::AlignUp(m_descriptor.m_pageSizeInBytes, Alignment::Image);
            
            // D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES for textures
            // D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES for render targets or depth stencils
            if (descriptor.m_resourceTypeFlags == ImageResourceTypeFlags::Image)
                m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
            else if (descriptor.m_resourceTypeFlags == ImageResourceTypeFlags::RenderTarget)
                m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
            else if (descriptor.m_resourceTypeFlags == ImageResourceTypeFlags::All)
                m_heapFlags = D3D12_HEAP_FLAG_DENY_BUFFERS;

            m_heapFlags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
        }

        RHI::Ptr<Heap> HeapFactory::CreateObject()
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();
            if (!heapMemoryUsage.TryReserveMemory(m_descriptor.m_pageSizeInBytes))
            {
                return nullptr;
            }

            AZ_PROFILE_SCOPE(RHI, "Create heap Page: size %dk", m_descriptor.m_pageSizeInBytes/1024);
           
            Device* device = static_cast<Device*>(m_descriptor.m_device);

            // use D3D12_HEAP_TYPE_DEFAULT for textures or render targets/depth stencil
            CD3DX12_HEAP_DESC heapDesc( m_descriptor.m_pageSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, 0, m_heapFlags);
                
            Microsoft::WRL::ComPtr<ID3D12Heap> heap;
            device->AssertSuccess(device->GetDevice()->CreateHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf())));
            heapMemoryUsage.m_residentInBytes += m_descriptor.m_pageSizeInBytes;
            return heap.Get();
        }

        void HeapFactory::ShutdownObject(Heap& object, bool isPoolShutdown)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = *m_descriptor.m_getHeapMemoryUsageFunction();

            AZ_Assert(heapMemoryUsage.m_residentInBytes >= m_descriptor.m_pageSizeInBytes && heapMemoryUsage.m_reservedInBytes >= m_descriptor.m_pageSizeInBytes, "Wrong heap memory usage");

            heapMemoryUsage.m_residentInBytes -= m_descriptor.m_pageSizeInBytes;
            heapMemoryUsage.m_reservedInBytes -= m_descriptor.m_pageSizeInBytes;

            if (isPoolShutdown)
            {
                m_descriptor.m_device->QueueForRelease(&object);
            }
        }

        bool HeapFactory::CollectObject(Heap& object)
        {
            (void)object;
            return m_descriptor.m_recycleOnCollect;
        }

        const HeapFactory::Descriptor& HeapFactory::GetDescriptor() const
        {
            return m_descriptor;
        }

        // PageTileAllocator
        void PageTileAllocator::Init(uint32_t totalTileCount)
        {
            m_totalTileCount = totalTileCount;
            m_allocatedTileCount = 0;
            m_freeList.push_back(Tiles(0, totalTileCount));
        }

        AZStd::list<Tiles> PageTileAllocator::TryAllocate(uint32_t tileCount, uint32_t& allocatedTileCount)
        {
            allocatedTileCount = 0;
            AZStd::list<Tiles> allocatedTiles;

            // Allocate desired amount of tiles or until the free list is empty
            while (allocatedTileCount < tileCount && m_freeList.size())
            {
                uint32_t tileNeeded = tileCount-allocatedTileCount;
                Tiles tiles = m_freeList.back();
                
                if (tiles.m_tileCount <= tileNeeded)
                {
                    // if the current node has tiles same or less than needed
                    // remove the node from the free list
                    m_freeList.pop_back();
                }
                else
                {
                    // the current node has more tiles than needed.
                    // split existing one. Keep the first half in the free list and use the second half for allocation 
                    tiles.m_offset = tiles.m_offset + tiles.m_tileCount - tileNeeded;
                    tiles.m_tileCount = tileNeeded;
                    m_freeList.back().m_tileCount = tiles.m_tileCount - tileNeeded;
                }

                allocatedTileCount += tiles.m_tileCount;
                allocatedTiles.push_back(tiles);
            }
            return allocatedTiles;
        }

        void PageTileAllocator::DeAllocate(const AZStd::vector<Tiles>& tilesList)
        {
            for (auto tiles : tilesList)
            {
                DeAllocate(tiles);
            }
        }

        void PageTileAllocator::DeAllocate(Tiles tiles)
        {
            struct Comp {
                bool operator()(const Tiles& a, const Tiles& b) const {
                  return a.m_offset < b.m_offset;
                }
            };

            // update tile count before the tiles get changed
            m_allocatedTileCount -= tiles.m_tileCount;

            auto itr = AZStd::upper_bound(AZStd::begin(m_freeList), AZStd::end(m_freeList), tiles, Comp());

            int64_t pos = itr - m_freeList.begin();

            if (pos > 0)
            {
                // check if the tile can merge with the element in front of 
                auto front = m_freeList.at(pos - 1);
                if (front.m_offset + front.m_tileCount == tiles.m_offset)
                {
                    tiles.m_offset = front.m_offset;
                    tiles.m_tileCount += front.m_tileCount;
                    // remvoe front 
                    m_freeList.erase(itr-1);
                }
            }

            if (pos < aznumeric_cast<int64_t>(m_freeList.size()))
            {
                // check if the tile can merge with the following element
                auto after = m_freeList.at(pos);
                if (tiles.m_offset + tiles.m_tileCount == after.m_offset)
                {
                    tiles.m_tileCount += after.m_tileCount;
                    // remvoe the following element
                    itr = m_freeList.erase(itr);
                }
            }

            m_freeList.insert(itr, tiles);
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

        // ------------- TileAllocator ------------- 
        void TileAllocator::Init(const Descriptor& descriptor, HeapAllocator& heapAllocator)
        {
            m_descriptor = descriptor;
            m_heapAllocator = &heapAllocator;

            m_tileCountPerPage = m_heapAllocator->GetPageSize()/descriptor.m_tileSizeInBytes;
        }

        uint32_t TileAllocator::AllocateFromFreeList(uint32_t tileCount, AZStd::vector<HeapTiles>& output)
        {
            uint32_t allocatedTileCount = 0;

            while ( !m_freeList.empty() && allocatedTileCount < tileCount)
            {
                uint32_t neededTileCount = tileCount - allocatedTileCount;
                auto heap = *m_freeList.begin();
                auto itr = m_pageContexts.find(heap);

                if (itr != m_pageContexts.end())
                {
                    uint32_t allocated = 0;
                    auto tilesGroups = itr->second.TryAllocate(neededTileCount, allocated);

                    if (itr->second.GetFreeTileCount() == 0)
                    {
                        m_freeList.erase(0);
                    }

                    if (allocated == 0)
                    {
                        AZ_Assert(false, "Implementation error: heap page in free list doesn't have tiles available");
                    }

                    // add tiles to result
                    HeapTiles heapTiles;
                    for (auto& tiles:tilesGroups)
                    {
                        heapTiles.m_tilesList.push_back(tiles);
                        heapTiles.m_totalTileCount += tiles.m_tileCount;
                    }
                    heapTiles.m_heap = itr->first;
                    output.emplace_back(heapTiles);
                }
                else
                {
                    AZ_Assert(false, "Implementation error: heap page context is missing.");
                }
            }
            return allocatedTileCount;
        }

        AZStd::vector<HeapTiles> TileAllocator::Allocate(uint32_t tileCount)
        {
            AZStd::vector<HeapTiles> tilesList;

            // Create new pages if there aren't enough free tiles available
            if (m_freeTileCount < tileCount)
            {
                uint32_t newPageCount = (tileCount - m_freeTileCount + m_tileCountPerPage - 1)/m_tileCountPerPage;

                for (uint32_t pageIdx = 0; pageIdx < newPageCount; pageIdx++)
                {
                    auto heap = m_heapAllocator->Allocate();
                    if (!heap)
                    {
                        // Return directly if we can't create more heaps
                        AZ_Error("TileAllocator", false, "Failed to create a heap page");
                        return AZStd::vector<HeapTiles>();
                    }

                    // setup page context for new heap page
                    auto& pageTileAllocator = m_pageContexts[heap];
                    pageTileAllocator.Init(m_tileCountPerPage);

                    // add page to free list
                    m_freeList.insert(heap);
                    m_freeTileCount += m_tileCountPerPage;
                }
            }

            // allocate from free list
            uint32_t allocatedTileCount = 0;
            allocatedTileCount = AllocateFromFreeList(tileCount, tilesList);
            AZ_Assert(allocatedTileCount == tileCount, "Implementation error: heap page context is missing.");
            m_freeTileCount -= tileCount;
            m_allocatedTileCount += tileCount;

            return tilesList;
        }

        void TileAllocator::DeAllocate(const AZStd::vector<HeapTiles>& tilesGroups)
        {
            for (auto& heapTiles : tilesGroups)
            {
                auto itr = m_pageContexts.find(heapTiles.m_heap);
                if (itr == m_pageContexts.end())
                {
                    AZ_Assert(false, "Heap wasn't allocated by this allocator");
                }
                else
                {
                    itr->second.DeAllocate(heapTiles.m_tilesList);
                    AZ_Assert(itr->second.GetFreeTileCount() > 0, "De-allocate tiles from heap failed");
                    m_freeList.insert(itr->first);

                    m_allocatedTileCount -= heapTiles.m_totalTileCount;
                }
            }
        }

        void TileAllocator::GarbageCollect()
        {
            auto itr = m_pageContexts.begin();
            while (itr != m_pageContexts.end())
            {
                if (itr->second.IsPageFree())
                {
                    m_freeList.erase(itr->first);
                    itr = m_pageContexts.erase(itr);
                    m_freeTileCount -= m_tileCountPerPage;
                }
                else
                {
                    itr++;
                }
            }

            m_heapAllocator->Collect();
        }

        void TileAllocator::Shutdown()
        {

        }
        
        uint32_t TileAllocator::GetAllocatedTileCount() const
        {
            return m_allocatedTileCount;
        }

    }
}

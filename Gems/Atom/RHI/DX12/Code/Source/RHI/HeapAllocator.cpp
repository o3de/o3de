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

        void PageTileAllocator::DeAllocate(AZStd::vector<Tiles>& tilesList)
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

            auto itr = AZStd::upper_bound(AZStd::begin(m_freeList), AZStd::end(m_freeList), tiles, Comp());

            auto pos = itr - m_freeList.begin();

            if (pos > 0)
            {
                auto front = m_freeList.at(pos - 1);
                if (front.m_offset + front.m_tileCount == tiles.m_offset)
                {
                    tiles.m_offset = front.m_offset;
                    tiles.m_tileCount += front.m_tileCount;
                    // remvoe front
                    m_freeList.erase(itr-1);
                }
            }

            if (pos < m_freeList.size())
            {
                auto after = m_freeList.at(pos);
                if (tiles.m_offset + tiles.m_tileCount == after.m_offset)
                {
                    tiles.m_tileCount += after.m_tileCount;
                    // remvoe after
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

        AZStd::vector<HeapTiles> TileAllocator::Allocate(size_t tileCount)
        {
            AZStd::vector<HeapTiles> tilesList;

            size_t pageIdx = 0;
            RHI::VirtualAddress address;

            size_t allocatedTileCount = 0;

            // Allocate from free list first
            while ( !m_freeList.empty() && allocatedTileCount < tileCount)
            {
                PageContext& pageContext = m_pageContexts[pageIdx];

                address = pageContext.m_allocator.Allocate(sizeInBytes, alignmentInBytes);
                if (address.IsValid())
                {
                    pageContext.m_inactiveCycleCount = 0;
                    break;
                }
            }

            /// Create a new page if none of our current ones can service the request.
            if (address.IsNull())
            {
                memory_type_pointer newPage = m_pageAllocator->Allocate();
                if (newPage == nullptr)
                {
                    return memory_allocation();
                }

                m_pages.emplace_back(newPage);
                m_pageContexts.emplace_back();

                PageContext& pageContext = m_pageContexts.back();
                pageContext.m_allocator.Init(m_descriptor);

                address = pageContext.m_allocator.Allocate(sizeInBytes, alignmentInBytes);
                AZ_Assert(address.IsValid(), "Failed to allocate from fresh page.");
            }
                        
            return memory_allocation(m_pages[pageIdx], address.m_ptr, sizeInBytes, alignmentInBytes);


            return tilesList;
        }

        void TileAllocator::DeAllocate(AZStd::vector<Tiles>& tiles)
        {

        }

        void TileAllocator::Shutdown()
        {

        }

        size_t TileAllocator::GetAllocatedTileCount() const
        {

        }

        size_t TileAllocator::GetAllocatedByteCount() const
        {

        }

        const Descriptor& TileAllocator::GetDescriptor() const
        {
            return m_descriptor;
        }

        void TileAllocator::FreeTiles(const Tile& tiles)
        {

        }

    }
}

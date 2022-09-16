/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/HeapAllocator.h>

#include <Atom/RHI/PageTileAllocator.h>

#include <AzCore/std/containers/set.h>

namespace AZ
{
    namespace DX12
    {
        //! A list of tile groups in a heap
        struct HeapTiles
        {
            //! The heap object which is evenly divided to multiple tiles
            RHI::Ptr<Heap> m_heap;
            //! Multiple tile spans. Each tile span represents a continous number of tiles in the heap
            AZStd::vector<RHI::PageTileSpan> m_tileSpanList;
            //! The total amount tiles in the m_tileSpanList
            uint32_t m_totalTileCount = 0;
        };

         //! An allocator which can allocate multiple tiles from multiple heap pages at once.
         //! It uses a HeapAllocator to allocate heap pages.
         //! It maintains a free list of free tiles. Each node of the list represents a set of continous tiles.
        class TileAllocator
        {
        public:
            using GetHeapMemoryUsageFunction = AZStd::function<RHI::HeapMemoryUsage*()>;

            struct Descriptor
            {
                uint32_t m_tileSizeInBytes = 0;
                GetHeapMemoryUsageFunction m_getHeapMemoryUsageFunction;
            };

            struct PageContext
            {
                RHI::PageTileAllocator m_pageTileAllocator;
                RHI::Ptr<Heap> m_heap;
            };

            AZ_CLASS_ALLOCATOR(TileAllocator, AZ::SystemAllocator, 0);

            TileAllocator() = default;

            void Init(const Descriptor& descriptor, HeapAllocator& heapAllocator);

            //! Allocate tiles. it may returen tiles from different heaps.
            AZStd::vector<HeapTiles> Allocate(uint32_t tileCount);

            //! DeAllocate multiple group of tiles 
            void DeAllocate(const AZStd::vector<HeapTiles>& tiles);

            //! reset the allocator to a state before initialization
            void Shutdown();

            //! get total number of tiles that could fit in the current set of allocated heaps
            uint32_t GetTotalTileCount() const;
            
            //! get the number of tiles currently in use
            uint32_t GetAllocatedTileCount() const;

            const Descriptor& GetDescriptor() const;

            //! Debug only. Print tile allocation info
            void DebugPrintInfo(const char* opName) const;

            //! Release free heap pages to HeapAllocator and run garbage collect for HeapAllocator
            //! It may release unused heap pages
            void GarbageCollect();

        private:            
            void AllocateFromFreeList(uint32_t tileCount, AZStd::vector<HeapTiles>& output);

            Descriptor m_descriptor;

            // the count of tiles in each heap page
            uint32_t m_tileCountPerPage = 0;

            // page tile allocator for each allocated heap page
            AZStd::unordered_map<RHI::Ptr<Heap>, RHI::PageTileAllocator> m_pageContexts;

            // a list of heaps which have free tiles
            AZStd::set<RHI::Ptr<Heap>> m_freeList;

            // Allocated tile count
            uint32_t m_allocatedTileCount = 0;
            // The total tile count from all allocated heaps
            uint32_t m_totalTileCount = 0;

            HeapAllocator* m_heapAllocator = nullptr;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryUsage.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/PageTileAllocator.h>
#include <Atom/RHI/PageTiles.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/set.h>

namespace AZ::RHI
{
    //! An allocator which can allocate multiple tiles from multiple heap pages at once.
    //! It uses a HeapAllocator to allocate heap pages.
    //! It maintains a free list of free tiles. Each node of the list represents a set of continuous tiles.
    //! The type for template should be an ObjectPoolTraits which provides heap type from its object type
    //! and traits for heap page allocator (ObjectPool)
    template<typename Traits> 
    class TileAllocator
    {
    public:
        using Heap = typename Traits::ObjectType;
        using HeapAllocator = ObjectPool<Traits>;
        using HeapTiles = PageTiles<Heap>;

        struct Descriptor
        {
            uint32_t m_tileSizeInBytes = 0;
            RHI::HeapMemoryUsage* m_heapMemoryUsage = nullptr;
        };

        struct PageContext
        {
            RHI::PageTileAllocator m_pageTileAllocator;
            RHI::Ptr<Heap> m_heap;
        };

        TileAllocator() = default;

        void Init(const Descriptor& descriptor, HeapAllocator& heapAllocator);

        //! Allocate tiles. it may return tiles from different heaps.
        AZStd::vector<HeapTiles> Allocate(uint32_t tileCount);

        //! Returns page memory allocation (in bytes) needed for required tile count.
        //! It returns 0 if there are enough tiles available within current allocated pages.
        size_t EvaluateMemoryAllocation(uint32_t tileCount);

        //! DeAllocate multiple group of tiles 
        void DeAllocate(const AZStd::vector<HeapTiles>& tiles);

        //! Reset the allocator to a state before initialization
        void Shutdown();

        //! Get total number of tiles that could fit in the current set of allocated heaps
        uint32_t GetTotalTileCount() const;
            
        //! Get the number of tiles currently in use
        uint32_t GetAllocatedTileCount() const;

        const Descriptor& GetDescriptor() const;

        //! Debug only. Print tile allocation info
        //! @param opName A string which usually be the caller function
        void DebugPrintInfo(const char* opName) const;

        //! Release free heap pages to HeapAllocator and run garbage collect for HeapAllocator
        //! It may release unused heap pages
        void GarbageCollect();

    private:            
        void AllocateFromFreeList(uint32_t tileCount, AZStd::vector<HeapTiles>& output);

        Descriptor m_descriptor;

        // The count of tiles in each heap page
        uint32_t m_tileCountPerPage = 0;

        // Page tile allocator for each allocated heap page
        AZStd::unordered_map<RHI::Ptr<Heap>, RHI::PageTileAllocator> m_pageContexts;

        // A list of heaps which have free tiles
        AZStd::set<RHI::Ptr<Heap>> m_freeList;

        // Allocated tile count
        uint32_t m_allocatedTileCount = 0;
        // The total tile count from all allocated heaps
        uint32_t m_totalTileCount = 0;

        HeapAllocator* m_heapAllocator = nullptr;
    };
}

#include <Atom/RHI/TileAllocator.inl>

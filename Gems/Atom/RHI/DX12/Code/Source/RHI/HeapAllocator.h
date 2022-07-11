/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Allocator.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/MemoryAllocation.h>
#include <Atom/RHI/MemorySubAllocator.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/PoolAllocator.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>
#include <RHI/Memory.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
                
        using Heap = ID3D12Heap;

        //! Allocators for allocating image resources (image or render target/depth/stencil)
        //! Image allocators use a object pool allocator to allocate a heap page (object)
        //! then use a sub alloctor to allocate the memory for the image from the a heap page

        // To allocate image resources, you need to have 
        //  a. A ObjectPool to allocate a heap page: HeapAllocator
        //  b. A PoolAllocator to do simple tile allocation from a heap page: TileAllocator
                
        enum class ImageResourceType : uint32_t
        {
            Image = 0,
            RenderTarget,
            Count
        };

        //! Flags to describe the resources supported by a heap.
        enum class ImageResourceTypeFlags : uint32_t
        {
            Image = AZ_BIT(static_cast<uint32_t>(ImageResourceType::Image)),
            RenderTarget = AZ_BIT(static_cast<uint32_t>(ImageResourceType::RenderTarget)),
            All = Image | RenderTarget
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::DX12::ImageResourceTypeFlags)

        // Factory which is responding to allocate heap pages from gpu
        class HeapFactory
            : public RHI::ObjectFactoryBase<Heap>
        {
        public:
            using GetHeapMemoryUsageFunction = AZStd::function<RHI::HeapMemoryUsage*()>;

            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_pageSizeInBytes = 0;
                GetHeapMemoryUsageFunction m_getHeapMemoryUsageFunction;
                bool m_recycleOnCollect = true;
                ImageResourceTypeFlags m_resourceTypeFlags;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<Heap> CreateObject();

            void ShutdownObject(Heap& object, bool isPoolShutdown);

            bool CollectObject(Heap& object);

            const Descriptor& GetDescriptor() const;

        private:
            Descriptor m_descriptor;
            D3D12_HEAP_FLAGS m_heapFlags;
        };

        class HeapAllocatorTraits
            : public RHI::ObjectPoolTraits
        {
        public:
            using ObjectType = Heap;
            using ObjectFactoryType = HeapFactory;
            using MutexType = AZStd::mutex;
        };

        class HeapAllocator
            : public RHI::ObjectPool<HeapAllocatorTraits>
        {
        public:
            size_t GetPageCount() const
            {
                return GetObjectCount();
            }

            uint32_t GetPageSize() const
            {
                return GetFactory().GetDescriptor().m_pageSizeInBytes;
            }
        };

        // a structure to represent continous number of tiles in a heap
        struct Tiles
        {
        public:
            Tiles(uint32_t offset, uint32_t count)
                : m_offset(offset)
                , m_tileCount(count)
            {
            }
            uint32_t m_offset;
            uint32_t m_tileCount;
        };

        class PageTileAllocator
        {
        public:
            void Init(uint32_t totalTileCount);

            // Allocate tiles. It returns tiles which are avaliable.
            // It may return tiles which less than desired count
            AZStd::list<Tiles> TryAllocate(uint32_t tileCount, /*out*/ uint32_t& allocatedTileCount);

            // DeAllocate multiple group of tiles 
            void DeAllocate(const AZStd::vector<Tiles>& tiles);

            // DeAllocate one group of tiles 
            void DeAllocate(Tiles tiles);

            uint32_t GetFreeTileCount() const;
            uint32_t GetUsedTileCount() const;
            uint32_t GetTotalTileCount() const;

            bool IsPageFree() const;

        private:
            uint32_t m_allocatedTileCount = 0;
            uint32_t m_totalTileCount = 0;

            //free tiles.
            std::vector<Tiles> m_freeList;
        };

        struct HeapTiles
        {
            RHI::Ptr<Heap> m_heap;
            AZStd::vector<Tiles> m_tilesList;
            uint32_t m_totalTileCount = 0;
        };

         //! An allocator which can allocate multiple tiles from multiple heap pages at once.
         //! It uses a HeapAllocator to allocate heap pages.
         //! It maintains a free list of free tiles. Each node of the list represents a set of continous tiles.
         //! When a set of tiles are de-allocated, it would be add to the free list. Merging would happen if the tiles
         //! are conjected to the tiles in the free list.
        class TileAllocator
        {
        public:
            struct Descriptor
            {
                uint32_t m_tileSizeInBytes = 0;
            };

            struct PageContext
            {
                PageTileAllocator m_pageTileAllocator;
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

            //! get total allocated tile count
            uint32_t GetAllocatedTileCount() const;

            //! Get heap memory usage
            uint32_t GetMemoryUsage() const;

            const Descriptor& GetDescriptor() const;

            //! Release free heap page to HeapAllocator and run garbage collect for HeapAllocator
            //! It may release unused heap ages
            void GarbageCollect();

        private:            
            uint32_t AllocateFromFreeList(uint32_t tileCount, AZStd::vector<HeapTiles>& output);

            Descriptor m_descriptor;

            // the count of tiles in each heap page
            uint32_t m_tileCountPerPage = 0;

            // tiles to be freed
            AZStd::queue<HeapTiles> m_garbage;
            
            AZStd::unordered_map<RHI::Ptr<Heap>, PageTileAllocator> m_pageContexts;

            // a list of heaps which have free tiles
            AZStd::set<RHI::Ptr<Heap>> m_freeList;

            // tile counts
            uint32_t m_allocatedTileCount = 0;
            uint32_t m_freeTileCount = 0;

            HeapAllocator* m_heapAllocator = nullptr;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <RHI/MemoryAllocator.h>
#include <RHI/TileAllocator.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class StreamingImagePool final
            : public RHI::StreamingImagePool
        {
            using Base = RHI::StreamingImagePool;
            friend class Image;

        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "0C123A3C-FBB7-4908-81A5-150D1DFE728A", Base);

            static RHI::Ptr<StreamingImagePool> Create();
            ~StreamingImagePool() = default;

        protected:
            // Allocate non-tiled memory with specified size and alignment
            // via the m_memoryAllocator
            MemoryView AllocateMemory(size_t size, size_t alignment);
            // Deallocate a non-tiled memory
            void DeAllocateMemory(MemoryView& memoryView);

            // Allocate memory blocks via the m_tileAllocator
            // @param [out] outHeapTiles The allocated heap tiles
            // @param blockCount The number of memory block count to be allocated
            // @return Returns RHI::ResultCode::Success if the allocation was successful.
            RHI::ResultCode AllocateMemoryBlocks(AZStd::vector<HeapTiles>& outHeapTiles, uint32_t blockCount);

            // Deallocate memory blocks            
            // @param heapTiles The heap tiles to be released. After the call, the vector is cleared.
            void DeAllocateMemoryBlocks(AZStd::vector<HeapTiles>& heapTiles);


        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::StreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::StreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::Image& image, uint32_t targetMipLevel) override;
            RHI::ResultCode SetMemoryBudgetInternal(size_t newBudget) override;
            bool SupportTiledImageInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            /// As streaming image tiles are allocated from a pool allocator, fragmentation will remain 0 for
            /// the streaming image pool
            void ComputeFragmentation() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            void WaitFinishUploading(const Image& image);
            
            // For allocating non-tile memory from heap pages
            // or unique allocation for large memory 
            MemoryAllocator m_memoryAllocator;

            // For allocating tiles from heap pages
            TileAllocator m_tileAllocator;
            MemoryPageAllocator m_heapPageAllocator;

            bool m_enableTileResource = false;
        };
    }
}

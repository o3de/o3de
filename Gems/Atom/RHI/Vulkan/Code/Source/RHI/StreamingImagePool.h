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
        class Image;

        class StreamingImagePool final
            : public RHI::StreamingImagePool
        {
            using Base = RHI::StreamingImagePool;

        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "0C123A3C-FBB7-4908-81A5-150D1DFE728A", Base);

            static RHI::Ptr<StreamingImagePool> Create();
            ~StreamingImagePool() = default;

            MemoryView AllocateMemory(size_t size, size_t alignment);
            RHI::ResultCode AllocateMemoryBlocks(AZStd::vector<HeapTiles>& outHeapTiles, uint32_t blockCount);
            void DeAllocateMemory(MemoryView& memoryView);
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
            
            // for allocating other non-tile memory from heap pages
            MemoryAllocator m_memoryAllocator;

            // for allocating tiles from heap pages
            TileAllocator m_tileAllocator;
            MemoryPageAllocator m_heapPageAllocator;

            bool m_enableTileResource = false;
        };
    }
}

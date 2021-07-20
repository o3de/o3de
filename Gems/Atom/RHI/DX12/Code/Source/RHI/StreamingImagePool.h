/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI/PoolAllocator.h>
#include <RHI/CommandList.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class Image;
        class StreamingImagePoolResolver;

        class StreamingImagePool final
            : public RHI::StreamingImagePool
        {
            using Base = RHI::StreamingImagePool;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "{D168A0F2-6B81-4281-9D4D-01C784F98DDD}", Base);

            static RHI::Ptr<StreamingImagePool> Create();

            Device& GetDevice() const;

            StreamingImagePoolResolver* GetResolver();

        private:
            StreamingImagePool() = default;

            D3D12_RESOURCE_ALLOCATION_INFO GetAllocationInfo(const RHI::ImageDescriptor& imageDescriptor, uint32_t residentMipLevel);

            //////////////////////////////////////////////////////////////////////////
            // RHI::StreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::StreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::Image& image, uint32_t targetMipLevel) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            void AllocateImageTilesInternal(Image& image, CommandList::TileMapRequest& request, uint32_t subresourceIndex);
            void DeAllocateImageTilesInternal(Image& image, uint32_t subresourceIndex);

            // Standard mips each have their own set of tiles.
            void AllocateStandardImageTiles(Image& image, RHI::Interval mipInterval);
            void DeAllocateStandardImageTiles(Image& image, RHI::Interval mipInterval);

            // Packed mips occupy a dedicated set of tiles.
            void AllocatePackedImageTiles(Image& image);

            RHI::Ptr<ID3D12Heap> m_heap;

            AZStd::mutex m_tileMutex;
            RHI::PoolAllocator m_tileAllocator;
        };
    }
}

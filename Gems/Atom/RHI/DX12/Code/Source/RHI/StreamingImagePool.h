/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <Atom/RHI/PoolAllocator.h>
#include <RHI/CommandList.h>
#include <RHI/HeapAllocator.h>
#include <RHI/TileAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class Image;
        class StreamingImagePoolResolver;

        class StreamingImagePool final
            : public RHI::DeviceStreamingImagePool
        {
            using Base = RHI::DeviceStreamingImagePool;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);
            AZ_RTTI(StreamingImagePool, "{D168A0F2-6B81-4281-9D4D-01C784F98DDD}", Base);

            static RHI::Ptr<StreamingImagePool> Create();

            Device& GetDevice() const;

            StreamingImagePoolResolver* GetResolver();

        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceStreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::DeviceImage& image, uint32_t targetMipLevel) override;
            RHI::ResultCode SetMemoryBudgetInternal(size_t newBudget) override;
            bool SupportTiledImageInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;

            // Streaming images are either committed resource or using tiles from heap pages. So there is no fragmentation
            void ComputeFragmentation() const override {}
            //////////////////////////////////////////////////////////////////////////

            // Check if we can use heap tiles for an image 
            bool ShouldUseTileHeap(const RHI::ImageDescriptor& imageDescriptor) const;

            // Allocate and map heap tiles for specified subresource of the image.
            // The allocated heap tiles will be saved in the image
            void AllocateImageTilesInternal(Image& image, uint32_t subresourceIndex);
            // Deallocate and unmap heap tiles for specified subresource of the image.
            // The heap tiles info for the image subresource is cleared. 
            void DeAllocateImageTilesInternal(Image& image, uint32_t subresourceIndex);

            // Standard mips each have their own set of tiles.
            void AllocateStandardImageTiles(Image& image, RHI::Interval mipInterval);
            void DeAllocateStandardImageTiles(Image& image, RHI::Interval mipInterval);

            // Packed mips occupy a dedicated set of tiles.
            void AllocatePackedImageTiles(Image& image);

            // Get the data reference of device heap memory usage 
            RHI::HeapMemoryUsage& GetDeviceHeapMemoryUsage();

            // A helper function that makes sure any previous upload request
            // is actually completed on @image.
            void WaitFinishUploading(const Image& image);

            // whether to enable tiled resource
            bool m_enableTileResource = false;

            // mutex to protect tile allocation and de-allocation from any threads
            AZStd::mutex m_tileMutex;

            // for allocating heap pages
            HeapAllocator m_heapPageAllocator;

            // for allocating tiles from heap pages
            TileAllocator m_tileAllocator;

            // The default tile for null tiles. The default tile is initialized in order to support images that have no tiles.
            // This is for resources when there is not enough of gpu memory
            // from https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_tiled_resources_tier
            // "GPU reads or writes to NULL mappings are undefined. Applications are encouraged to workaround this limitation by
            // repeatedly mapping the same page to everywhere a NULL mapping would've been used."
            HeapTiles m_defaultTile;
        };
    }
}

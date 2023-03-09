/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/Fence.h>
#include <RHI/Device.h>

#define AZ_RHI_VULKAN_USE_TILED_RESOURCES

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<StreamingImagePool> StreamingImagePool::Create()
        {
            return aznew StreamingImagePool();
        }
        
        MemoryView StreamingImagePool::AllocateMemory(size_t size, size_t alignment)
        {
            return m_memoryAllocator.Allocate(size, alignment);
        }

        RHI::ResultCode StreamingImagePool::AllocateMemoryBlocks(AZStd::vector<HeapTiles>& outHeapTiles, uint32_t blockCount)
        {
            AZ_Assert(outHeapTiles.empty(), "outHeapTiles should be empty");
            AZ_Assert(blockCount, "Try to allocate 0 block");

             // Check if heap memory is enough for the tiles.
            RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            size_t pageAllocationInBytes = m_tileAllocator.EvaluateMemoryAllocation(blockCount);

            // Try to release some memory if there isn't enough memory available in the pool
            bool canAllocate = heapMemoryUsage.CanAllocate(pageAllocationInBytes);
            if (!canAllocate && m_memoryReleaseCallback)
            {
                bool releaseSuccess = false;
                while (!canAllocate)
                {
                    // Request to release some memory
                    releaseSuccess = m_memoryReleaseCallback();

                    // break out of the loop if memory release did not happen
                    if (!releaseSuccess)
                    {
                        break;
                    }

                    // re-evaluation page memory allocation since there are tiles were released.
                    pageAllocationInBytes = m_tileAllocator.EvaluateMemoryAllocation(blockCount);
                    canAllocate = heapMemoryUsage.CanAllocate(pageAllocationInBytes);
                }

                if (!releaseSuccess)
                {
                    AZ_Warning("Vulkan::StreamingImagePool", false, "There isn't enough memory."
                        "Try increase the StreamingImagePool memory budget");
                }
            }

            outHeapTiles = m_tileAllocator.Allocate(blockCount);
            if (outHeapTiles.size() == 0)
            {
                return RHI::ResultCode::OutOfMemory;
            }
            else
            {
                return RHI::ResultCode::Success;
            }
        }

        void StreamingImagePool::DeAllocateMemory(MemoryView& memoryView)
        {
            m_memoryAllocator.DeAllocate(memoryView);
        }

        void StreamingImagePool::DeAllocateMemoryBlocks(AZStd::vector<HeapTiles>& heapTiles)
        {
            m_tileAllocator.DeAllocate(heapTiles);
            heapTiles.clear();
            m_tileAllocator.GarbageCollect();
        }

        RHI::ResultCode StreamingImagePool::InitInternal(RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor)
        {
            auto& device = static_cast<Device&>(deviceBase);

            m_enableTileResource = device.GetFeatures().m_tiledResource;

#ifndef AZ_RHI_VULKAN_USE_TILED_RESOURCES
            // Disable tile resource for all 
            m_enableTileResource = false;
#endif

            uint32_t memoryTypeBits = 0;
            {
                // Use an image descriptor of size 1x1 to get the memory requirements.
                RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
                RHI::ImageDescriptor imageDescriptor = RHI::ImageDescriptor::Create2D(bindFlags, 1, 1, RHI::Format::R8G8B8A8_UNORM);
                VkMemoryRequirements memRequirements = device.GetImageMemoryRequirements(imageDescriptor);
                memoryTypeBits = memRequirements.memoryTypeBits;
            }
           
            RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            MemoryAllocator::Descriptor memoryAllocDescriptor;
            memoryAllocDescriptor.m_device = &device;
            memoryAllocDescriptor.m_pageSizeInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_imagePoolPageSizeInBytes;
            memoryAllocDescriptor.m_heapMemoryLevel = heapMemoryLevel;
            memoryAllocDescriptor.m_memoryTypeBits = memoryTypeBits;
            memoryAllocDescriptor.m_getHeapMemoryUsageFunction = [&]() { return &heapMemoryUsage; };
            memoryAllocDescriptor.m_recycleOnCollect = false;
            memoryAllocDescriptor.m_collectLatency = 0;
            m_memoryAllocator.Init(memoryAllocDescriptor);

            // Initialize tile allocator
            m_heapPageAllocator.Init(memoryAllocDescriptor);
            const uint32_t TileSizeInBytes = SparseImageInfo::StandardBlockSize;
            TileAllocator::Descriptor tileAllocatorDesc;
            tileAllocatorDesc.m_tileSizeInBytes = TileSizeInBytes;
            // Tile allocator updates used resident memory
            tileAllocatorDesc.m_heapMemoryUsage = &m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            m_tileAllocator.Init(tileAllocatorDesc, m_heapPageAllocator);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::StreamingImageInitRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;
            imageDescriptor.m_sharedQueueMask |= RHI::HardwareQueueClassMask::Copy;
            const uint16_t expectedResidentMipLevel = static_cast<uint16_t>(request.m_descriptor.m_mipLevels - request.m_tailMipSlices.size());

            // Initialize the image first so we can get accurate memory requirements
            RHI::ResultCode result = image.Init(device, imageDescriptor, m_enableTileResource);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("Vulkan:StreamingImagePool", false, "Failed to create image");
                return result;
            }

            result = image.AllocateAndBindMemory(*this, expectedResidentMipLevel);

            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("Vulkan:StreamingImagePool", false, "Failed to allocate or bind memory for image");
                return result;
            }

            // Queue upload tail mip slices and wait for it finished.
            RHI::StreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = &image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;
            device.GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            // update resident mip level
            image.SetStreamedMipLevel(expectedResidentMipLevel);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            WaitFinishUploading(image);

            const uint16_t residentMipLevelBefore = static_cast<uint16_t>(image.GetResidentMipLevel());
            const uint16_t residentMipLevelAfter = residentMipLevelBefore - static_cast<uint16_t>(request.m_mipSlices.size());
            
            RHI::ResultCode result = image.AllocateAndBindMemory(*this, residentMipLevelAfter);

            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("Vulkan:StreamingImagePool", false, "Failed to allocate or bind memory for image");
                return result;
            }

            // Create new expand request and append callback from the StreamingImagePool.
            RHI::StreamingImageExpandRequest newRequest = request;
            newRequest.m_completeCallback = [=]()
            {
                Image& imageCompleted = static_cast<Image&>(*request.m_image);
                imageCompleted.FinalizeAsyncUpload(residentMipLevelAfter);

                request.m_completeCallback();
            };

            device.GetAsyncUploadQueue().QueueUpload(newRequest, residentMipLevelBefore);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::TrimImageInternal(RHI::Image& imageBase, uint32_t targetMipLevel)
        {
            auto& image = static_cast<Image&>(imageBase);

            WaitFinishUploading(image);

            RHI::ResultCode result = image.TrimImage(*this, aznumeric_caster(targetMipLevel), true);
            return result;
        }

        void StreamingImagePool::ShutdownInternal()
        {
            m_memoryAllocator.Shutdown();
        }

        void StreamingImagePool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            auto& image = static_cast<Image&>(resourceBase);

            WaitFinishUploading(image);

            image.ReleaseAllMemory(*this);
            image.Invalidate();
        }

        void StreamingImagePool::ComputeFragmentation() const
        {
            const RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_fragmentation = m_memoryAllocator.ComputeFragmentation();
        }

        void StreamingImagePool::OnFrameEnd()
        {
            m_memoryAllocator.GarbageCollect();
            Base::OnFrameEnd();
        }

        void StreamingImagePool::WaitFinishUploading(const Image& image)
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.GetAsyncUploadQueue().WaitForUpload(image.GetUploadHandle());
        }

        void StreamingImagePool::SetNameInternal(const AZStd::string_view& name)
        {
            m_memoryAllocator.SetName(AZ::Name{ name });
        }

        RHI::ResultCode StreamingImagePool::SetMemoryBudgetInternal(size_t newBudget)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);

            if (newBudget == 0)
            {
                heapMemoryUsage.m_budgetInBytes = newBudget;
                return RHI::ResultCode::Success;
            }

            // Can't set to new budget if the new budget is smaller than allocated and there is no memory release handling
            if (newBudget < heapMemoryUsage.m_totalResidentInBytes && !m_memoryReleaseCallback)
            {
                AZ_Warning("StreamingImagePool", false, "Can't set pool memory budget to %u because the memory release callback wasn't set", newBudget);
                return RHI::ResultCode::InvalidArgument;
            }

            bool releaseSuccess = true;
            while (newBudget < heapMemoryUsage.m_totalResidentInBytes && releaseSuccess)
            {
                size_t previousTotalResidentInBytes = heapMemoryUsage.m_totalResidentInBytes;
                size_t previousUsedResidentInBytes = heapMemoryUsage.m_usedResidentInBytes;

                // Request to release some memory
                releaseSuccess = m_memoryReleaseCallback();

                // Ensure there were memory released in the m_memoryReleaseCallback() call
                if (releaseSuccess)
                {
                    releaseSuccess = previousTotalResidentInBytes > heapMemoryUsage.m_totalResidentInBytes
                        || previousUsedResidentInBytes > heapMemoryUsage.m_usedResidentInBytes;
                    if (!releaseSuccess)
                    {
                        AZ_Warning("StreamingImagePool", false, "bad release function");
                    }
                }
            }

            // Failed to release memory to desired budget. Set current budget to current total resident.
            if (!releaseSuccess)
            {
                heapMemoryUsage.m_budgetInBytes = heapMemoryUsage.m_totalResidentInBytes;
                AZ_Warning("StreamingImagePool", false, "Failed to set pool memory budget to %u, set to %u instead", newBudget, heapMemoryUsage.m_budgetInBytes);
            }
            else
            {
                heapMemoryUsage.m_budgetInBytes = newBudget;
            }
            return RHI::ResultCode::Success;
        }
        
        bool StreamingImagePool::SupportTiledImageInternal() const
        {
            return m_enableTileResource;
        }
    }
}


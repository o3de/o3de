/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Conversion.h>
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

        RHI::ResultCode StreamingImagePool::AllocateMemoryBlocks(
            uint32_t blockCount,
            const VkMemoryRequirements& memReq,
            AZStd::vector<RHI::Ptr<VulkanMemoryAllocation>>& outAllocatedBlocks)
        {
            AZ_Assert(outAllocatedBlocks.empty(), "outAllocatedBlocks should be empty");
            AZ_Assert(blockCount, "Try to allocate 0 block");

            // Check if heap memory is enough for the tiles.
            RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            size_t neededMemoryInBytes = memReq.alignment * blockCount;

            // Try to release some memory if there isn't enough memory available in the pool
            bool canAllocate = heapMemoryUsage.CanAllocate(neededMemoryInBytes);
            if (!canAllocate && m_memoryReleaseCallback)
            {
                size_t targetUsage = heapMemoryUsage.m_budgetInBytes - neededMemoryInBytes;
                bool releaseSuccess = m_memoryReleaseCallback(targetUsage);

                if (!releaseSuccess)
                {
                    AZ_Warning("Vulkan::StreamingImagePool", false, "There isn't enough memory."
                        "Try increase the StreamingImagePool memory budget");
                }
            }

            AZStd::vector<VmaAllocation> allocations(blockCount);
            VmaAllocationCreateInfo allocCreateInfo = GetVmaAllocationCreateInfo(RHI::HeapMemoryLevel::Device);

            auto& device = static_cast<Device&>(GetDevice());
            VkResult vkResult = vmaAllocateMemoryPages(
                device.GetVmaAllocator(),
                &memReq,
                &allocCreateInfo,
                blockCount,
                allocations.data(),
                nullptr);

            AssertSuccess(vkResult);
            if (vkResult == VK_SUCCESS)
            {
                // Initialize all vma allocations as Vulkan::Allocations
                // Each element is a separated allocation.
                AZStd::transform(
                    allocations.begin(),
                    allocations.end(),
                    AZStd::back_inserter(outAllocatedBlocks),
                    [&](const auto& alloc)
                    {
                        RHI::Ptr<VulkanMemoryAllocation> memAlloc = VulkanMemoryAllocation::Create();
                        memAlloc->Init(device, alloc);
                        return memAlloc;
                    });

                heapMemoryUsage.m_usedResidentInBytes += neededMemoryInBytes;
                heapMemoryUsage.m_totalResidentInBytes += neededMemoryInBytes;
            }
            return ConvertResult(vkResult);
        }

        void StreamingImagePool::DeAllocateMemoryBlocks(AZStd::vector<RHI::Ptr<VulkanMemoryAllocation>>& blocks)
        {
            auto& device = static_cast<Device&>(GetDevice());
            size_t usedMem = 0;
            AZStd::for_each(
                blocks.begin(),
                blocks.end(),
                [&](auto& alloc)
                {
                    usedMem += alloc->GetSize();
                    device.QueueForRelease(alloc);
                });

            RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            heapMemoryUsage.m_usedResidentInBytes -= usedMem;
            heapMemoryUsage.m_totalResidentInBytes -= usedMem;
            blocks.clear();
        }        

        RHI::ResultCode StreamingImagePool::InitInternal(RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor)
        {
            auto& device = static_cast<Device&>(deviceBase);

            m_enableTileResource = device.GetFeatures().m_tiledResource;

#ifndef AZ_RHI_VULKAN_USE_TILED_RESOURCES
            // Disable tile resource for all 
            m_enableTileResource = false;
#endif 
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;
            imageDescriptor.m_sharedQueueMask |= RHI::HardwareQueueClassMask::Copy;
            const uint16_t expectedResidentMipLevel = static_cast<uint16_t>(request.m_descriptor.m_mipLevels - request.m_tailMipSlices.size());
            Image::InitFlags flags = m_enableTileResource ? Image::InitFlags::TrySparse : Image::InitFlags::None;

            RHI::ResultCode result = image.Init(device, imageDescriptor, flags);
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

            if (image.m_memoryView.IsValid())
            {
                RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
                heapMemoryUsage.m_usedResidentInBytes += image.m_memoryView.GetSize();
                heapMemoryUsage.m_totalResidentInBytes += image.m_memoryView.GetSize();
            }

            // Queue upload tail mip slices and wait for it finished.
            RHI::DeviceStreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = request.m_image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;
            device.GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            // update resident mip level
            image.SetStreamedMipLevel(expectedResidentMipLevel);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request) 
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
            RHI::DeviceStreamingImageExpandRequest newRequest = request;
            newRequest.m_completeCallback = [=]()
            {
                Image& imageCompleted = static_cast<Image&>(*request.m_image);
                imageCompleted.FinalizeAsyncUpload(residentMipLevelAfter);

                request.m_completeCallback();
            };

            device.GetAsyncUploadQueue().QueueUpload(newRequest, residentMipLevelBefore);
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::TrimImageInternal(RHI::DeviceImage& imageBase, uint32_t targetMipLevel)
        {
            auto& image = static_cast<Image&>(imageBase);

            WaitFinishUploading(image);

            RHI::ResultCode result = image.TrimImage(*this, aznumeric_caster(targetMipLevel), true);
            return result;
        }

        void StreamingImagePool::ShutdownInternal()
        {
        }

        void StreamingImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            auto& image = static_cast<Image&>(resourceBase);

            WaitFinishUploading(image);

            image.ReleaseAllMemory(*this);
            image.Invalidate();
        }

        void StreamingImagePool::ComputeFragmentation() const
        {
        }

        void StreamingImagePool::WaitFinishUploading(const Image& image)
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.GetAsyncUploadQueue().WaitForUpload(image.GetUploadHandle());
        }

        RHI::ResultCode StreamingImagePool::SetMemoryBudgetInternal(size_t newBudget)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            
            if (newBudget == 0)
            {
                return RHI::ResultCode::Success;
            }

            // Can't set to new budget if the new budget is smaller than allocated and there is no memory release handling
            if (newBudget < heapMemoryUsage.m_usedResidentInBytes && !m_memoryReleaseCallback)
            {
                AZ_Warning("StreamingImagePool", false, "Can't set pool memory budget to %u because the memory release callback wasn't set", newBudget);
                return RHI::ResultCode::InvalidArgument;
            }

            bool releaseSuccess = true;
            // If the new budget is smaller than the memory are in use, we need to release some memory
            if (newBudget < heapMemoryUsage.m_usedResidentInBytes)
            {
                releaseSuccess = m_memoryReleaseCallback(newBudget);
            }

            if (!releaseSuccess)
            {
                heapMemoryUsage.m_budgetInBytes = heapMemoryUsage.m_usedResidentInBytes;
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


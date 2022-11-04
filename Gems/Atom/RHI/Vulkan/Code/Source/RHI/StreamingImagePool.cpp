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
            m_memoryAllocatorUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            MemoryAllocator::Descriptor memoryAllocDescriptor;
            memoryAllocDescriptor.m_device = &device;
            memoryAllocDescriptor.m_pageSizeInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_imagePoolPageSizeInBytes;
            memoryAllocDescriptor.m_heapMemoryLevel = heapMemoryLevel;
            memoryAllocDescriptor.m_memoryTypeBits = memoryTypeBits;
            // We don't pass the ResourcePool MemoryUsage because we will manually control residency when expanding/trimming.
            memoryAllocDescriptor.m_getHeapMemoryUsageFunction = [this]() { return &m_memoryAllocatorUsage; };
            memoryAllocDescriptor.m_recycleOnCollect = true;
            memoryAllocDescriptor.m_collectLatency = RHI::Limits::Device::FrameCountMax;
            m_memoryAllocator.Init(memoryAllocDescriptor);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::StreamingImageInitRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;
            imageDescriptor.m_sharedQueueMask |= RHI::HardwareQueueClassMask::Copy;

            // Initialize the image first so we can get accurate memory requirements
            RHI::ResultCode result = image.Init(device, imageDescriptor, m_enableTileResource);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("Vulkan:StreamingImagePool", false, "Failed to create image");
                return result;
            }

            const uint16_t expectedResidentMipLevel = static_cast<uint16_t>(request.m_descriptor.m_mipLevels - request.m_tailMipSlices.size());
            result = image.AllocateAndBindMemory(m_memoryAllocator, expectedResidentMipLevel);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("Vulkan:StreamingImagePool", false, "Failed to allocate or bind memory for image");
                return result;
                
            }

            // Queue upload tail mip slices.
            RHI::StreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = &image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;

            device.GetAsyncUploadQueue().QueueUpload(uploadMipRequest, expectedResidentMipLevel);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            WaitFinishUploading(image);

            const uint16_t residentMipLevelBefore = static_cast<uint16_t>(image.GetResidentMipLevel());
            const uint16_t residentMipLevelAfter = residentMipLevelBefore - static_cast<uint16_t>(request.m_mipSlices.size());
            const VkMemoryRequirements memoryRequirements = GetMemoryRequirements(image.GetDescriptor(), residentMipLevelAfter);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            const size_t imageSizeBefore = image.GetResidentSizeInBytes();
            const size_t imageSizeAfter = memoryRequirements.size;

            // Try reserve memory for the increased size.
            if (!memoryUsage.TryReserveMemory(imageSizeAfter - imageSizeBefore))
            {
                return RHI::ResultCode::OutOfMemory;
            }

            memoryUsage.m_residentInBytes += imageSizeAfter - imageSizeBefore;
            memoryUsage.Validate();

            // Create new expand request and append callback from the StreamingImagePool.
            RHI::StreamingImageExpandRequest newRequest = request;
            newRequest.m_completeCallback = [=]()
            {
                Image& imageCompleted = static_cast<Image&>(*request.m_image);
                imageCompleted.SetResidentSizeInBytes(imageSizeAfter);
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

            // Set streamed mip level to target mip level.
            if (image.GetStreamedMipLevel() < targetMipLevel)
            {
                image.SetStreamedMipLevel(static_cast<uint16_t>(targetMipLevel));
            }

            const VkMemoryRequirements memoryRequirements = GetMemoryRequirements(image.GetDescriptor(), targetMipLevel);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            const size_t imageSizeBefore = image.GetResidentSizeInBytes();
            const size_t imageSizeAfter = memoryRequirements.size;

            const size_t sizeDiffInBytes = imageSizeBefore - imageSizeAfter;
            memoryUsage.m_residentInBytes -= sizeDiffInBytes;
            memoryUsage.m_reservedInBytes -= sizeDiffInBytes;
            image.SetResidentSizeInBytes(imageSizeAfter);

            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownInternal()
        {
            m_memoryAllocator.Shutdown();
        }

        void StreamingImagePool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            auto& image = static_cast<Image&>(resourceBase);

            WaitFinishUploading(image);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_residentInBytes -= image.GetResidentSizeInBytes();
            memoryUsage.m_reservedInBytes -= image.GetResidentSizeInBytes();
            memoryUsage.Validate();

            m_memoryAllocator.DeAllocate(image.m_memoryView);
            image.m_memoryView = MemoryView();
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
                // Request to release some memory
                releaseSuccess = m_memoryReleaseCallback();
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
    }
}

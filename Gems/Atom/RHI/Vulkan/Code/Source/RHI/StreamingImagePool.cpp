/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_precompiled.h"
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

            const uint32_t expectedResidentMipLevel = request.m_descriptor.m_mipLevels - static_cast<uint32_t>(request.m_tailMipSlices.size());
            const VkMemoryRequirements memoryRequirements = GetMemoryRequirements(imageDescriptor, expectedResidentMipLevel);

            // [GFX TODO][ATOM-973] Add support for sparse residency for streaming textures. Now we allocate the whole mipmap chain.
            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(m_memoryAllocator.GetDescriptor().m_heapMemoryLevel);
            if (!memoryUsage.TryReserveMemory(memoryRequirements.size))
            {
                return RHI::ResultCode::OutOfMemory;
            }

            RHI::ResultCode result = image.Init(device, imageDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            result = image.BindMemoryView(
                m_memoryAllocator.Allocate(image.m_memoryRequirements.size, image.m_memoryRequirements.alignment), 
                m_memoryAllocator.GetDescriptor().m_heapMemoryLevel);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            image.SetStreamedMipLevel(static_cast<uint16_t>(expectedResidentMipLevel));
            image.SetResidentSizeInBytes(memoryRequirements.size);

            // Queue upload tail mip slices.
            RHI::StreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = &image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;

            device.GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            memoryUsage.m_residentInBytes += memoryRequirements.size;
            memoryUsage.Validate();

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) 
        {
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            WaitFinishUploading(image);

            const uint16_t residentMipLevelBefore = image.GetResidentMipLevel();
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
                image.SetStreamedMipLevel(targetMipLevel);
            }

            const VkMemoryRequirements memoryRequirements = GetMemoryRequirements(image.GetDescriptor(), targetMipLevel);
            const uint16_t residentMipLevelBefore = image.GetResidentMipLevel();

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

        void StreamingImagePool::OnFrameEnd()
        {
            m_memoryAllocator.GarbageCollect();
            Base::OnFrameEnd();
        }

        VkMemoryRequirements StreamingImagePool::GetMemoryRequirements(const RHI::ImageDescriptor& imageDescriptor, uint32_t residentMipLevel)
        {
            auto& device = static_cast<Device&>(GetDevice());
            uint32_t alignment = GetFormatDimensionAlignment(imageDescriptor.m_format);

            RHI::ImageDescriptor residentImageDescriptor = imageDescriptor;
            residentImageDescriptor.m_size = imageDescriptor.m_size.GetReducedMip(residentMipLevel);
            residentImageDescriptor.m_size.m_width = RHI::AlignUp(residentImageDescriptor.m_size.m_width, alignment);
            residentImageDescriptor.m_size.m_height = RHI::AlignUp(residentImageDescriptor.m_size.m_height, alignment);
            residentImageDescriptor.m_mipLevels = imageDescriptor.m_mipLevels - residentMipLevel;

            return device.GetImageMemoryRequirements(imageDescriptor);
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
    }
}

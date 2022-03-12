/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<ImagePool> ImagePool::Create()
        {
            return aznew ImagePool();
        }

        void ImagePool::GarbageCollect()
        {
            m_memoryAllocator.GarbageCollect();
        }

        void ImagePool::OnFrameEnd()
        {
            GarbageCollect();
            Base::OnFrameEnd();
        }

        RHI::ResultCode ImagePool::InitInternal(RHI::Device& deviceBase, const RHI::ImagePoolDescriptor& descriptorBase)
        {
            auto& device = static_cast<Device&>(deviceBase);

            VkDeviceSize imagePageSizeInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_imagePoolPageSizeInBytes;
            RHI::HeapMemoryLevel heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            if (const auto* descriptor = azrtti_cast<const ImagePoolDescriptor*>(&descriptorBase))
            {
                imagePageSizeInBytes = descriptor->m_imagePageSizeInBytes;
                heapMemoryLevel = descriptor->m_heapMemoryLevel;
            }

            uint32_t memoryTypeBits = 0;
            {
                // Use an image descriptor of size 1x1 to get the memory requirements.
                RHI::ImageDescriptor imageDescriptor = RHI::ImageDescriptor::Create2D(descriptorBase.m_bindFlags, 1, 1, RHI::Format::R8G8B8A8_UNORM);
                VkMemoryRequirements memRequirements = device.GetImageMemoryRequirements(imageDescriptor);
                memoryTypeBits = memRequirements.memoryTypeBits;
            }

            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(heapMemoryLevel);
            MemoryAllocator::Descriptor memoryAllocDescriptor;
            memoryAllocDescriptor.m_device = &device;
            memoryAllocDescriptor.m_pageSizeInBytes = static_cast<size_t>(imagePageSizeInBytes);
            memoryAllocDescriptor.m_heapMemoryLevel = heapMemoryLevel;
            memoryAllocDescriptor.m_memoryTypeBits = memoryTypeBits;
            memoryAllocDescriptor.m_getHeapMemoryUsageFunction = [&]() { return &heapMemoryUsage; };
            memoryAllocDescriptor.m_recycleOnCollect = true;
            memoryAllocDescriptor.m_collectLatency = RHI::Limits::Device::FrameCountMax;
            m_memoryAllocator.Init(memoryAllocDescriptor);

            SetResolver(AZStd::make_unique<ImagePoolResolver>(static_cast<Device&>(device)));

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode ImagePool::InitImageInternal(const RHI::ImageInitRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Image* image = static_cast<Image*>(request.m_image);
            AZ_Assert(image, "Null image during initialization.");

            RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
            // Add copy write flag since images can be copy into or clear using a command list.
            imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;

            RHI::ResultCode result = image->Init(device, imageDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            MemoryView memoryView = m_memoryAllocator.Allocate(image->m_memoryRequirements.size, image->m_memoryRequirements.alignment);
            result = image->BindMemoryView(
                memoryView,
                m_memoryAllocator.GetDescriptor().m_heapMemoryLevel);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            image->SetResidentSizeInBytes(memoryView.GetSize());
            return result;
        }

        RHI::ResultCode ImagePool::UpdateImageContentsInternal(const RHI::ImageUpdateRequest& request) 
        {
            size_t bytesTransferred = 0;
            RHI::ResultCode resultCode = static_cast<ImagePoolResolver*>(GetResolver())->UpdateImage(request, bytesTransferred);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_memoryUsage.m_transferPull.m_bytesPerFrame += bytesTransferred;
            }
            return resultCode;
        }
        
        void ImagePool::ShutdownInternal()
        {
            m_memoryAllocator.Shutdown();
        }

        void ImagePool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            auto& image = static_cast<Image&>(resourceBase);

            m_memoryAllocator.DeAllocate(image.m_memoryView);
            image.m_memoryView = MemoryView();
            image.Invalidate();
        }

        void ImagePool::SetNameInternal(const AZStd::string_view& name)
        {
            m_memoryAllocator.SetName(AZ::Name{ name });
        }
    }
}

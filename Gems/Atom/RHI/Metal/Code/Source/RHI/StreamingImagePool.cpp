/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/Casting/lossy_cast.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/StreamingImagePoolResolver.h>

namespace AZ
{
    namespace Metal
    {        
        RHI::Ptr<StreamingImagePool> StreamingImagePool::Create()
        {
            return aznew StreamingImagePool();
        }

        Device& StreamingImagePool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }
        
        StreamingImagePoolResolver* StreamingImagePool::GetResolver()
        {
            return static_cast<StreamingImagePoolResolver*>(Base::GetResolver());
        }
        
        RHI::ResultCode StreamingImagePool::InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor)
        {
            AZ_PROFILE_FUNCTION(RHI);
            
            Device& device = static_cast<Device&>(deviceBase);
            SetResolver(AZStd::make_unique<StreamingImagePoolResolver>(device, this));
            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownInternal()
        {
        }

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request)
        {
            Image& image = static_cast<Image&>(*request.m_image);

            MemoryView memoryView = GetDevice().CreateImageCommitted(image.GetDescriptor());
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::OutOfMemory;
            }

            memoryView.SetName(image.GetName().GetStringView());
            image.m_memoryView = AZStd::move(memoryView);
            image.m_streamedMipLevel = request.m_descriptor.m_mipLevels - static_cast<uint32_t>(request.m_tailMipSlices.size());

            // Queue upload tail mip slices
             RHI::DeviceStreamingImageExpandRequest uploadMipRequest;
             uploadMipRequest.m_image = request.m_image;
             uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
             uploadMipRequest.m_waitForUpload = true;
             GetDevice().GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            return RHI::ResultCode::Success;
        }
        
        void StreamingImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            auto& image = static_cast<Image&>(resourceBase);
            auto& device = static_cast<Device&>(GetDevice());

            device.GetAsyncUploadQueue().WaitForUpload(image.GetUploadHandle());
            if (auto* resolver = GetResolver())
            {
                ResourcePoolResolver* poolResolver = static_cast<ResourcePoolResolver*>(resolver);
                poolResolver->OnResourceShutdown(resourceBase);
            }
 
            GetDevice().QueueForRelease(image.GetMemoryView());
            image.m_memoryView = {};            
        }
        
        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request)
        {
            AZ_PROFILE_FUNCTION(RHI);
            auto& image = static_cast<Image&>(*request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            device.GetAsyncUploadQueue().WaitForUpload(image.GetUploadHandle());
            
            const uint16_t residentMipLevelBefore = image.GetResidentMipLevel();
            const uint16_t residentMipLevelAfter = residentMipLevelBefore - static_cast<uint16_t>(request.m_mipSlices.size());
            
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
            auto& device = static_cast<Device&>(GetDevice());

            device.GetAsyncUploadQueue().WaitForUpload(image.GetUploadHandle());

            // Set streamed mip level to target mip level.
            if (image.GetStreamedMipLevel() < targetMipLevel)
            {
                image.SetStreamedMipLevel(targetMipLevel);
            }

            return RHI::ResultCode::Success;
        }
    }
}

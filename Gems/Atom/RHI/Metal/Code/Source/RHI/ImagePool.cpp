/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/BufferPool.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ImagePool> ImagePool::Create()
        {
            return aznew ImagePool();
        }
        
        Device& ImagePool::GetDevice() const
        {
            return reinterpret_cast<Device&>(Base::GetDevice());
        }

        RHI::ResultCode ImagePool::InitInternal(RHI::Device& deviceBase, const RHI::ImagePoolDescriptor&)
        {
            Device& device = static_cast<Device&>(deviceBase);
            SetResolver(AZStd::make_unique<ImagePoolResolver>(device));
            return RHI::ResultCode::Success;
        }
        
        RHI::ResultCode ImagePool::InitImageInternal(const RHI::DeviceImageInitRequest& request)
        {
            Image& image = static_cast<Image&>(*request.m_image);
            
            //Super simple implementation. Just creates a committed resource for the image. No
            //real pooling happening at the moment. Other approaches might involve creating dedicated
            //heaps and then placing resources onto those heaps. This will allow us to control residency
            //at the heap level.
            MemoryView memoryView = GetDevice().CreateImageCommitted(image.GetDescriptor(), MTLStorageModePrivate);
            if (!memoryView.IsValid())
            {
                return RHI::ResultCode::OutOfMemory;
            }
            
            memoryView.SetName(image.GetName().GetStringView());
            image.m_memoryView = AZStd::move(memoryView);
            return RHI::ResultCode::Success;
        }
        
        RHI::ResultCode ImagePool::UpdateImageContentsInternal(const RHI::DeviceImageUpdateRequest& request)
        {
            size_t bytesTransferred = 0;
            RHI::ResultCode resultCode = GetResolver()->UpdateImage(request, bytesTransferred);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_memoryUsage.m_transferPull.m_bytesPerFrame += bytesTransferred;
            }
            return resultCode;
        }
        
        void ImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }

            Image& image = static_cast<Image&>(resourceBase);
            GetDevice().QueueForRelease(image.GetMemoryView());
            image.m_memoryView = {};
            image.m_pendingResolves = 0;
        }
    
        ImagePoolResolver* ImagePool::GetResolver()
        {
            return static_cast<ImagePoolResolver*>(Base::GetResolver());
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ImagePool> ImagePool::Create()
    {
        return aznew ImagePool();
    }

    RHI::ResultCode ImagePool::InitInternal(RHI::Device& deviceBase, [[maybe_unused]] const RHI::ImagePoolDescriptor& descriptorBase)
    {
        auto& device = static_cast<Device&>(deviceBase);
        SetResolver(AZStd::make_unique<ImagePoolResolver>(device));
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode ImagePool::InitImageInternal(const RHI::DeviceImageInitRequest& request)
    {
        auto& device = static_cast<Device&>(GetDevice());
        Image* image = static_cast<Image*>(request.m_image);
        AZ_Assert(image, "Null image during initialization.");

        RHI::ImageDescriptor imageDescriptor = request.m_descriptor;
        // Add copy write flag since images can be copy into or clear using a command list.
        imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::CopyWrite;

        RHI::ResultCode result = image->Init(device, imageDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        return result;
    }

    RHI::ResultCode ImagePool::UpdateImageContentsInternal(const RHI::DeviceImageUpdateRequest& request)
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
    }

    void ImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
    {
        auto& image = static_cast<Image&>(resourceBase);

        if (auto* resolver = GetResolver())
        {
            ResourcePoolResolver* poolResolver = static_cast<ResourcePoolResolver*>(resolver);
            poolResolver->OnResourceShutdown(resourceBase);
        }

        image.Invalidate();
    }
}

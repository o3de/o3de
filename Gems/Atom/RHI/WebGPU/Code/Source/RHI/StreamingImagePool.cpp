/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/StreamingImagePool.h>

namespace AZ::WebGPU
{
    RHI::Ptr<StreamingImagePool> StreamingImagePool::Create()
    {
        return aznew StreamingImagePool();
    }

    RHI::ResultCode StreamingImagePool::InitInternal(
        [[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor)
    {
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

        RHI::ResultCode result = image.Init(device, imageDescriptor);
        if (result != RHI::ResultCode::Success)
        {
            AZ_Warning("StreamingImagePool", false, "Failed to create image");
            return result;
        }

        // Queue upload tail mip slices and wait for it finished.
        RHI::DeviceStreamingImageExpandRequest uploadMipRequest;
        uploadMipRequest.m_image = &image;
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

        RHI::ResultCode result = image.TrimImage(aznumeric_caster(targetMipLevel));
        return result;
    }

    void StreamingImagePool::ShutdownInternal()
    {
    }

    void StreamingImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
    {
        auto& image = static_cast<Image&>(resourceBase);
        WaitFinishUploading(image);
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
}

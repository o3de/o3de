/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceImagePool.h>

namespace AZ::RHI
{
    ResultCode DeviceImagePool::Init(Device& device, const ImagePoolDescriptor& descriptor)
    {
        return DeviceResourcePool::Init(
            device, descriptor,
            [this, &device, &descriptor]()
        {
            /**
                * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                * possibility that users will get garbage values from GetDescriptor().
                */
            m_descriptor = descriptor;

            return InitInternal(device, descriptor);
        });
    }

    bool DeviceImagePool::ValidateUpdateRequest(const DeviceImageUpdateRequest& updateRequest) const
    {
        if (Validation::IsEnabled())
        {
            const ImageDescriptor& imageDescriptor = updateRequest.m_image->GetDescriptor();
            if (updateRequest.m_imageSubresource.m_mipSlice >= imageDescriptor.m_mipLevels ||
                updateRequest.m_imageSubresource.m_arraySlice >= imageDescriptor.m_arraySize)
            {
                AZ_Error("DeviceImagePool", false,
                    "Updating subresource (array: %d, mip: %d), but the image dimensions are (arraySize: %d, mipLevels: %d)",
                    updateRequest.m_imageSubresource.m_mipSlice,
                    updateRequest.m_imageSubresource.m_arraySlice,
                    imageDescriptor.m_arraySize,
                    imageDescriptor.m_mipLevels);
                return false;
            }
        }

        AZ_UNUSED(updateRequest);
        return true;
    }

    ResultCode DeviceImagePool::InitImage(const DeviceImageInitRequest& initRequest)
    {
        return DeviceImagePoolBase::InitImage(
            initRequest.m_image,
            initRequest.m_descriptor,
            [this, &initRequest]() { return InitImageInternal(initRequest); });
    }

    ResultCode DeviceImagePool::UpdateImageContents(const DeviceImageUpdateRequest& request)
    {
        if (!ValidateIsInitialized() || !ValidateNotProcessingFrame())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(request.m_image))
        {
            return ResultCode::InvalidArgument;
        }

        if (!ValidateUpdateRequest(request))
        {
            return ResultCode::InvalidArgument;
        }

        return UpdateImageContentsInternal(request);
    }

    const ImagePoolDescriptor& DeviceImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void DeviceImagePool::ComputeFragmentation() const
    {
        // Currently, images are not suballocated within a heap and are instead created as committed resources. This method should be
        // implemented when a suballocation strategy for image pools is implemented.
    }
}

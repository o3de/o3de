/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MultiDeviceImagePool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    ResultCode MultiDeviceImagePool::Init(MultiDevice::DeviceMask deviceMask, const ImagePoolDescriptor& descriptor)
    {
        return MultiDeviceResourcePool::Init(
            deviceMask,
            [this, &descriptor]()
            {
                // Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                // for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                // possibility that users will get garbage values from GetDescriptor().
                m_descriptor = descriptor;

                IterateDevices(
                    [this, &descriptor](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateImagePool();
                        GetDeviceImagePool(deviceIndex)->Init(*device, descriptor);

                        return true;
                    });

                return ResultCode::Success;
            });
    }

    bool MultiDeviceImagePool::ValidateUpdateRequest(const MultiDeviceImageUpdateRequest& updateRequest) const
    {
        if (Validation::IsEnabled())
        {
            const ImageDescriptor& imageDescriptor = updateRequest.m_image->GetDescriptor();
            if (updateRequest.m_imageSubresource.m_mipSlice >= imageDescriptor.m_mipLevels ||
                updateRequest.m_imageSubresource.m_arraySlice >= imageDescriptor.m_arraySize)
            {
                AZ_Error(
                    "MultiDeviceImagePool",
                    false,
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

    ResultCode MultiDeviceImagePool::InitImage(const MultiDeviceImageInitRequest& initRequest)
    {
        return MultiDeviceImagePoolBase::InitImage(
            initRequest.m_image,
            initRequest.m_descriptor,
            [this, &initRequest]()
            {
                ResultCode result = IterateObjects<SingleDeviceImagePool>([&initRequest](auto deviceIndex, auto deviceImagePool)
                {
                    if (!initRequest.m_image->m_deviceObjects.contains(deviceIndex))
                    {
                        initRequest.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();
                    }

                    SingleDeviceImageInitRequest imageInitRequest(
                        *initRequest.m_image->GetDeviceImage(deviceIndex), initRequest.m_descriptor, initRequest.m_optimizedClearValue);
                    return deviceImagePool->InitImage(imageInitRequest);
                });

                if (result != ResultCode::Success)
                {
                    // Reset already initialized device-specific ImagePools and set deviceMask to 0
                    m_deviceObjects.clear();
                    MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
                }

                return result;
            });
    }

    ResultCode MultiDeviceImagePool::UpdateImageContents(const MultiDeviceImageUpdateRequest& request)
    {
        if (!ValidateIsInitialized())
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

        return IterateObjects<SingleDeviceImagePool>([&request](auto deviceIndex, auto deviceImagePool)
        {
            SingleDeviceImageUpdateRequest imageUpdateRequest;

            imageUpdateRequest.m_image = request.m_image->GetDeviceImage(deviceIndex).get();
            imageUpdateRequest.m_imageSubresource = request.m_imageSubresource;
            imageUpdateRequest.m_imageSubresourcePixelOffset = request.m_imageSubresourcePixelOffset;
            imageUpdateRequest.m_sourceData = request.m_sourceData;
            imageUpdateRequest.m_sourceSubresourceLayout = request.m_sourceSubresourceLayout.GetDeviceImageSubresource(deviceIndex);

            return deviceImagePool->UpdateImageContents(imageUpdateRequest);
        });
    }

    const ImagePoolDescriptor& MultiDeviceImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void MultiDeviceImagePool::Shutdown()
    {
        IterateObjects<SingleDeviceImagePool>([]([[maybe_unused]] auto deviceIndex, auto deviceImagePool)
        {
            deviceImagePool->Shutdown();
        });

        MultiDeviceResourcePool::Shutdown();
    }
} // namespace AZ::RHI

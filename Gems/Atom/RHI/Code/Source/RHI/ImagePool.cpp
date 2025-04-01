/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    ResultCode ImagePool::Init(const ImagePoolDescriptor& descriptor)
    {
        return ResourcePool::Init(
            descriptor.m_deviceMask,
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

    bool ImagePool::ValidateUpdateRequest(const ImageUpdateRequest& updateRequest) const
    {
        if (Validation::IsEnabled())
        {
            const ImageDescriptor& imageDescriptor = updateRequest.m_image->GetDescriptor();
            if (updateRequest.m_imageSubresource.m_mipSlice >= imageDescriptor.m_mipLevels ||
                updateRequest.m_imageSubresource.m_arraySlice >= imageDescriptor.m_arraySize)
            {
                AZ_Error(
                    "ImagePool",
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

    ResultCode ImagePool::InitImage(const ImageInitRequest& initRequest)
    {
        return ImagePoolBase::InitImage(
            initRequest.m_image,
            initRequest.m_descriptor,
            [this, &initRequest]()
            {
                initRequest.m_image->Init(GetDeviceMask() & initRequest.m_deviceMask);

                ResultCode result = IterateObjects<DeviceImagePool>(
                    [&initRequest](auto deviceIndex, auto deviceImagePool)
                    {
                        if (CheckBit(initRequest.m_image->GetDeviceMask(), deviceIndex))
                        {
                            if (!initRequest.m_image->m_deviceObjects.contains(deviceIndex))
                            {
                                initRequest.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();
                            }

                            DeviceImageInitRequest imageInitRequest(
                                *initRequest.m_image->GetDeviceImage(deviceIndex),
                                initRequest.m_descriptor,
                                initRequest.m_optimizedClearValue);
                            return deviceImagePool->InitImage(imageInitRequest);
                        }
                        else
                        {
                            initRequest.m_image->m_deviceObjects.erase(deviceIndex);
                        }

                        return ResultCode::Success;
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

    ResultCode ImagePool::UpdateImageDeviceMask(const ImageDeviceMaskRequest& request)
    {
        return IterateObjects<DeviceImagePool>(
            [&request](auto deviceIndex, auto deviceImagePool)
            {
                if (CheckBit(request.m_deviceMask, deviceIndex))
                {
                    if (!request.m_image->m_deviceObjects.contains(deviceIndex))
                    {
                        request.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();

                        DeviceImageInitRequest imageInitRequest(
                            *request.m_image->GetDeviceImage(deviceIndex), request.m_image->m_descriptor, request.m_optimizedClearValue);
                        auto result = deviceImagePool->InitImage(imageInitRequest);

                        if (result == ResultCode::Success)
                        {
                            request.m_image->Init(SetBit(request.m_image->GetDeviceMask(), deviceIndex));
                        }

                        return result;
                    }
                }
                else
                {
                    request.m_image->Init(ResetBit(request.m_image->GetDeviceMask(), deviceIndex));
                    request.m_image->m_deviceObjects.erase(deviceIndex);
                }

                return ResultCode::Success;
            });
    }

    ResultCode ImagePool::UpdateImageContents(const ImageUpdateRequest& request)
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

        return IterateObjects<DeviceImagePool>([&request](auto deviceIndex, auto deviceImagePool)
        {
            DeviceImageUpdateRequest imageUpdateRequest;

            imageUpdateRequest.m_image = request.m_image->GetDeviceImage(deviceIndex).get();
            imageUpdateRequest.m_imageSubresource = request.m_imageSubresource;
            imageUpdateRequest.m_imageSubresourcePixelOffset = request.m_imageSubresourcePixelOffset;
            imageUpdateRequest.m_sourceData = request.m_sourceData;
            imageUpdateRequest.m_sourceSubresourceLayout = request.m_sourceSubresourceLayout.GetDeviceImageSubresource(deviceIndex);

            return deviceImagePool->UpdateImageContents(imageUpdateRequest);
        });
    }

    const ImagePoolDescriptor& ImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void ImagePool::Shutdown()
    {
        ResourcePool::Shutdown();
    }
} // namespace AZ::RHI

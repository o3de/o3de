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

namespace AZ
{
    namespace RHI
    {
        MultiDeviceImageInitRequest::MultiDeviceImageInitRequest(
            MultiDeviceImage& image, const ImageDescriptor& descriptor, const ClearValue* optimizedClearValue)
            : m_image{ &image }
            , m_descriptor{ descriptor }
            , m_optimizedClearValue{ optimizedClearValue }
        {
        }

        ResultCode MultiDeviceImagePool::Init(MultiDevice::DeviceMask deviceMask, const ImagePoolDescriptor& descriptor)
        {
            return MultiDeviceResourcePool::Init(
                deviceMask,
                [this, &descriptor]()
                {
                    /**
                     * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                     * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                     * possibility that users will get garbage values from GetDescriptor().
                     */
                    m_descriptor = descriptor;

                    IterateDevices(
                        [this, &descriptor](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceImagePools[deviceIndex] = Factory::Get().CreateImagePool();
                            m_deviceImagePools[deviceIndex]->Init(*device, descriptor);

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
                    ResultCode result = ResultCode::Success;

                    for (auto& [deviceIndex, deviceImagePool] : m_deviceImagePools)
                    {
                        if (!initRequest.m_image->m_deviceImages.contains(deviceIndex))
                            initRequest.m_image->m_deviceImages[deviceIndex] = Factory::Get().CreateImage();
                        ImageInitRequest imageInitRequest(
                            *initRequest.m_image->m_deviceImages[deviceIndex], initRequest.m_descriptor, initRequest.m_optimizedClearValue);
                        result = deviceImagePool->InitImage(imageInitRequest);

                        if (result != ResultCode::Success)
                            break;
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

            ResultCode result = ResultCode::Success;

            for (auto& [deviceIndex, deviceImagePool] : m_deviceImagePools)
            {
                ImageUpdateRequest imageUpdateRequest;

                imageUpdateRequest.m_image = request.m_image->m_deviceImages[deviceIndex].get();
                imageUpdateRequest.m_imageSubresource = request.m_imageSubresource;
                imageUpdateRequest.m_imageSubresourcePixelOffset = request.m_imageSubresourcePixelOffset;
                imageUpdateRequest.m_sourceData = request.m_sourceData;
                imageUpdateRequest.m_sourceSubresourceLayout = request.m_sourceSubresourceLayout.GetDeviceImageSubresource(deviceIndex);

                result = deviceImagePool->UpdateImageContents(imageUpdateRequest);

                if (result != ResultCode::Success)
                    break;
            }

            return result;
        }

        const ImagePoolDescriptor& MultiDeviceImagePool::GetDescriptor() const
        {
            return m_descriptor;
        }

        void MultiDeviceImagePool::Shutdown()
        {
            for (auto [_, pool] : m_deviceImagePools)
                pool->Shutdown();
            MultiDeviceResourcePool::Shutdown();
        }
    } // namespace RHI
} // namespace AZ

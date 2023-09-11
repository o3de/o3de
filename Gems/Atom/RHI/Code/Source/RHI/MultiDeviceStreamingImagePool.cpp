/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MultiDeviceStreamingImagePool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::RHI
{
    bool MultiDeviceStreamingImagePool::ValidateInitRequest(const MultiDeviceStreamingImageInitRequest& initRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (initRequest.m_tailMipSlices.empty())
            {
                AZ_Error(
                    "MultiDeviceStreamingImagePool",
                    false,
                    "No tail mip slices were provided. You must provide at least one tail mip slice.");
                return false;
            }

            if (initRequest.m_tailMipSlices.size() > initRequest.m_descriptor.m_mipLevels)
            {
                AZ_Error("MultiDeviceStreamingImagePool", false, "Tail mip array exceeds the number of mip levels in the image.");
                return false;
            }

            // Streaming images are only allowed to update via the CPU.
            if (CheckBitsAny(
                    initRequest.m_descriptor.m_bindFlags,
                    ImageBindFlags::Color | ImageBindFlags::DepthStencil | ImageBindFlags::ShaderWrite))
            {
                AZ_Error("MultiDeviceStreamingImagePool", false, "Streaming images may only contain read-only bind flags.");
                return false;
            }
        }

        AZ_UNUSED(initRequest);
        return true;
    }

    bool MultiDeviceStreamingImagePool::ValidateExpandRequest(const MultiDeviceStreamingImageExpandRequest& expandRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (!ValidateIsRegistered(expandRequest.m_image))
            {
                return false;
            }
        }

        AZ_UNUSED(expandRequest);
        return true;
    }

    ResultCode MultiDeviceStreamingImagePool::Init(MultiDevice::DeviceMask deviceMask, const StreamingImagePoolDescriptor& descriptor)
    {
        AZ_PROFILE_FUNCTION(RHI);

        return MultiDeviceResourcePool::Init(
            deviceMask,
            [this, &descriptor]()
            {
                // Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                // for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                // possibility that users will get garbage values from GetDescriptor().
                m_descriptor = descriptor;

                ResultCode result = ResultCode::Success;

                IterateDevices(
                    [this, &descriptor, &result](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateStreamingImagePool();
                        result = GetDeviceStreamingImagePool(deviceIndex)->Init(*device, descriptor);

                        return result == ResultCode::Success;
                    });

                if (result != ResultCode::Success)
                {
                    // Reset already initialized device-specific StreamingImagePool and set deviceMask to 0
                    m_deviceObjects.clear();
                    MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
                }

                return ResultCode::Success;
            });
    }

    ResultCode MultiDeviceStreamingImagePool::InitImage(const MultiDeviceStreamingImageInitRequest& initRequest)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateInitRequest(initRequest))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = MultiDeviceImagePoolBase::InitImage(
            initRequest.m_image,
            initRequest.m_descriptor,
            [this, &initRequest]()
            {
                return IterateObjects<StreamingImagePool>([&initRequest](auto deviceIndex, auto deviceStreamingImagePool)
                {
                    initRequest.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();
                    StreamingImageInitRequest streamingImageInitRequest(
                        *initRequest.m_image->GetDeviceImage(deviceIndex), initRequest.m_descriptor, initRequest.m_tailMipSlices);
                    return deviceStreamingImagePool->InitImage(streamingImageInitRequest);
                });
            });

        AZ_Warning("MultiDeviceStreamingImagePool", resultCode == ResultCode::Success, "Failed to initialize image.");
        return resultCode;
    }

    ResultCode MultiDeviceStreamingImagePool::ExpandImage(const MultiDeviceStreamingImageExpandRequest& request)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateExpandRequest(request))
        {
            return ResultCode::InvalidArgument;
        }

        auto expandCount = AZStd::make_shared<AZStd::atomic_int>(static_cast<int>(m_deviceObjects.size()));

        auto completeCallback = [expandCount, request]()
        {
            if (--*expandCount == 0)
            {
                request.m_completeCallback();
            }
        };

        return IterateObjects<StreamingImagePool>([&request, &completeCallback](auto deviceIndex, auto deviceStreamingImagePool)
        {
            StreamingImageExpandRequest expandRequest;

            expandRequest.m_image = request.m_image->GetDeviceImage(deviceIndex).get();
            expandRequest.m_mipSlices = request.m_mipSlices;
            expandRequest.m_waitForUpload = request.m_waitForUpload;
            expandRequest.m_completeCallback = completeCallback;

            return deviceStreamingImagePool->ExpandImage(expandRequest);
        });
    }

    ResultCode MultiDeviceStreamingImagePool::TrimImage(MultiDeviceImage& image, uint32_t targetMipLevel)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(&image))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = IterateObjects<StreamingImagePool>([&image, targetMipLevel](auto deviceIndex, auto deviceStreamingImagePool)
        {
            return deviceStreamingImagePool->TrimImage(*image.GetDeviceImage(deviceIndex), targetMipLevel);
        });

        if (resultCode == ResultCode::Success)
        {
            // If initialization succeeded, assign the new resident mip level. Invalidate resource views
            // so that they no longer reference trimmed mip levels.
            image.InvalidateViews();
        }

        return resultCode;
    }

    const StreamingImagePoolDescriptor& MultiDeviceStreamingImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }
} // namespace AZ::RHI

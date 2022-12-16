/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/StreamingImagePool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        StreamingImageInitRequest::StreamingImageInitRequest(
            Image& image, const ImageDescriptor& descriptor, AZStd::span<const StreamingImageMipSlice> tailMipSlices)
            : m_image{ &image }
            , m_descriptor{ descriptor }
            , m_tailMipSlices{ tailMipSlices }
        {
        }

        bool StreamingImagePool::ValidateInitRequest(const StreamingImageInitRequest& initRequest) const
        {
            if (Validation::IsEnabled())
            {
                if (initRequest.m_tailMipSlices.empty())
                {
                    AZ_Error("StreamingImagePool", false, "No tail mip slices were provided. You must provide at least one tail mip slice.");
                    return false;
                }

                if (initRequest.m_tailMipSlices.size() > initRequest.m_descriptor.m_mipLevels)
                {
                    AZ_Error("StreamingImagePool", false, "Tail mip array exceeds the number of mip levels in the image.");
                    return false;
                }

                // Streaming images are only allowed to update via the CPU.
                if (CheckBitsAny(
                    initRequest.m_descriptor.m_bindFlags,
                    ImageBindFlags::Color | ImageBindFlags::DepthStencil | ImageBindFlags::ShaderWrite))
                {
                    AZ_Error("StreamingImagePool", false, "Streaming images may only contain read-only bind flags.");
                    return false;
                }
            }

            AZ_UNUSED(initRequest);
            return true;
        }

        bool StreamingImagePool::ValidateExpandRequest(const StreamingImageExpandRequest& expandRequest) const
        {
            if (Validation::IsEnabled())
            {
                if (!ValidateIsRegistered(expandRequest.m_image))
                {
                    return false;
                }

                if (expandRequest.m_image->GetResidentMipLevel() < expandRequest.m_mipSlices.size())
                {
                    AZ_Error("StreamingImagePool", false, "Attempted to expand image more than the number of mips available.");
                    return false;
                }
            }

            AZ_UNUSED(expandRequest);
            return true;
        }

        ResultCode StreamingImagePool::Init(DeviceMask deviceMask, const StreamingImagePoolDescriptor& descriptor)
        {
            AZ_PROFILE_FUNCTION(RHI);

            return ResourcePool::Init(
                deviceMask,
                descriptor,
                [this, /*TODO: &deviceMask,*/ &descriptor]()
                {
                    /**
                     * Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                     * for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                     * possibility that users will get garbage values from GetDescriptor().
                     */
                    m_descriptor = descriptor;

                    ResultCode result = ResultCode::Success;

                    IterateDevices(
                        [this, &descriptor, &result](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceStreamingImagePools[deviceIndex] = Factory::Get().CreateStreamingImagePool();
                            result = m_deviceStreamingImagePools[deviceIndex]->Init(*device, descriptor);

                            return result == ResultCode::Success;
                        });

                    return ResultCode::Success;
                });
        }

        ResultCode StreamingImagePool::InitImage(const StreamingImageInitRequest& initRequest)
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

            ResultCode resultCode = ImagePoolBase::InitImage(
                initRequest.m_image,
                initRequest.m_descriptor,
                [this, &initRequest]()
                {
                    ResultCode result = ResultCode::Success;

                    for (auto& [deviceIndex, deviceStreamingImagePool] : m_deviceStreamingImagePools)
                    {
                        initRequest.m_image->m_deviceImages[deviceIndex] = Factory::Get().CreateImage();
                        DeviceStreamingImageInitRequest streamingImageInitRequest(
                            *initRequest.m_image->m_deviceImages[deviceIndex], initRequest.m_descriptor, initRequest.m_tailMipSlices);
                        result = deviceStreamingImagePool->InitImage(streamingImageInitRequest);

                        if (result != ResultCode::Success)
                            break;
                    }

                    return result;
                });

            if (resultCode == ResultCode::Success)
            {
                // If initialization succeeded, assign the new resident mip level.
                // TODO: unused m_residentMipLevel...
                initRequest.m_image->m_residentMipLevel = static_cast<uint32_t>(initRequest.m_descriptor.m_mipLevels - initRequest.m_tailMipSlices.size());
            }

            AZ_Warning("StreamingImagePool", resultCode == ResultCode::Success, "Failed to initialize image.");
            return resultCode;
        }

        ResultCode StreamingImagePool::ExpandImage(const StreamingImageExpandRequest& request)
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateExpandRequest(request))
            {
                return ResultCode::InvalidArgument;
            }

            ResultCode resultCode = ResultCode::Success;

            for (auto& [deviceIndex, deviceStreamingImagePool] : m_deviceStreamingImagePools)
            {
                DeviceStreamingImageExpandRequest expandRequest;

                expandRequest.m_image = request.m_image->m_deviceImages[deviceIndex].get();
                expandRequest.m_mipSlices = request.m_mipSlices;
                expandRequest.m_waitForUpload = request.m_waitForUpload;
                expandRequest.m_completeCallback =
                    request.m_completeCallback; // TODO: this should only be called once, when all devices are done

                resultCode = deviceStreamingImagePool->ExpandImage(expandRequest);

                if (resultCode != ResultCode::Success)
                    break;
            }

            if (resultCode == ResultCode::Success)
            {
                request.m_image->m_residentMipLevel -= static_cast<uint32_t>(request.m_mipSlices.size());
            }
            return resultCode;
        }

        ResultCode StreamingImagePool::TrimImage(Image& image, uint32_t targetMipLevel)
        {
            if (!ValidateIsInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateIsRegistered(&image))
            {
                return ResultCode::InvalidArgument;
            }

            if (image.m_residentMipLevel < targetMipLevel)
            {
                ResultCode resultCode = ResultCode::Success;

                for (auto& [deviceIndex, deviceStreamingImagePool] : m_deviceStreamingImagePools)
                {
                    resultCode = deviceStreamingImagePool->TrimImage(*image.m_deviceImages[deviceIndex], targetMipLevel);

                    if (resultCode != ResultCode::Success)
                        break;
                }

                if (resultCode == ResultCode::Success)
                {
                    // If initialization succeeded, assign the new resident mip level. Invalidate resource views
                    // so that they no longer reference trimmed mip levels.
                    image.m_residentMipLevel = targetMipLevel;
                    image.InvalidateViews();
                }
                return resultCode;
            }

            return RHI::ResultCode::Success;
        }

        const StreamingImagePoolDescriptor& StreamingImagePool::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}

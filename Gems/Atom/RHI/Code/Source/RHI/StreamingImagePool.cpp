/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/StreamingImagePool.h>

#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        StreamingImageInitRequest::StreamingImageInitRequest(
            Image& image,
            const ImageDescriptor& descriptor,
            AZStd::array_view<StreamingImageMipSlice> tailMipSlices)
            : m_image{&image}
            , m_descriptor{descriptor}
            , m_tailMipSlices{tailMipSlices}
        {}
        
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

        ResultCode StreamingImagePool::Init(Device& device, const StreamingImagePoolDescriptor& descriptor)
        {
            AZ_TRACE_METHOD();

            return ResourcePool::Init(
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

        ResultCode StreamingImagePool::InitImage(const StreamingImageInitRequest& initRequest)
        {
            AZ_TRACE_METHOD();

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
                return InitImageInternal(initRequest);
            });

            if (resultCode == ResultCode::Success)
            {
                // If initialization succeeded, assign the new resident mip level.
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

            const ResultCode resultCode = ExpandImageInternal(request);
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
                const ResultCode resultCode = TrimImageInternal(image, targetMipLevel);
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

        ResultCode StreamingImagePool::InitInternal(Device&, const StreamingImagePoolDescriptor&)
        {
            return ResultCode::Success;
        }

        ResultCode StreamingImagePool::InitImageInternal(const StreamingImageInitRequest&)
        {
            return ResultCode::Unimplemented;
        }

        ResultCode StreamingImagePool::ExpandImageInternal(const StreamingImageExpandRequest&)
        {
            return ResultCode::Unimplemented;
        }

        ResultCode StreamingImagePool::TrimImageInternal(Image&, uint32_t)
        {
            return ResultCode::Unimplemented;
        }
    }
}

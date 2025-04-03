/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceStreamingImagePool.h>

namespace AZ::RHI
{
    bool DeviceStreamingImagePool::ValidateInitRequest(const DeviceStreamingImageInitRequest& initRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (initRequest.m_tailMipSlices.empty())
            {
                AZ_Error("DeviceStreamingImagePool", false, "No tail mip slices were provided. You must provide at least one tail mip slice.");
                return false;
            }

            if (initRequest.m_tailMipSlices.size() > initRequest.m_descriptor.m_mipLevels)
            {
                AZ_Error("DeviceStreamingImagePool", false, "Tail mip array exceeds the number of mip levels in the image.");
                return false;
            }

            // Streaming images are only allowed to update via the CPU.
            if (CheckBitsAny(
                initRequest.m_descriptor.m_bindFlags,
                ImageBindFlags::Color | ImageBindFlags::DepthStencil | ImageBindFlags::ShaderWrite))
            {
                AZ_Error("DeviceStreamingImagePool", false, "Streaming images may only contain read-only bind flags.");
                return false;
            }
        }

        AZ_UNUSED(initRequest);
        return true;
    }

    bool DeviceStreamingImagePool::ValidateExpandRequest(const DeviceStreamingImageExpandRequest& expandRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (!ValidateIsRegistered(expandRequest.m_image.get()))
            {
                return false;
            }

            if (expandRequest.m_image->GetResidentMipLevel() < expandRequest.m_mipSlices.size())
            {
                AZ_Error("DeviceStreamingImagePool", false, "Attempted to expand image more than the number of mips available.");
                return false;
            }
        }

        AZ_UNUSED(expandRequest);
        return true;
    }

    ResultCode DeviceStreamingImagePool::Init(Device& device, const StreamingImagePoolDescriptor& descriptor)
    {
        AZ_PROFILE_FUNCTION(RHI);
        SetName(AZ::Name("DeviceStreamingImagePool"));
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

    ResultCode DeviceStreamingImagePool::InitImage(const DeviceStreamingImageInitRequest& initRequest)
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

        ResultCode resultCode = DeviceImagePoolBase::InitImage(
            initRequest.m_image.get(),
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

        AZ_Warning("DeviceStreamingImagePool", resultCode == ResultCode::Success, "Failed to initialize image.");
        return resultCode;
    }

    ResultCode DeviceStreamingImagePool::ExpandImage(const DeviceStreamingImageExpandRequest& request)
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

    ResultCode DeviceStreamingImagePool::TrimImage(DeviceImage& image, uint32_t targetMipLevel)
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

    const StreamingImagePoolDescriptor& DeviceStreamingImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }

    void DeviceStreamingImagePool::SetLowMemoryCallback(LowMemoryCallback callback)
    {
        m_memoryReleaseCallback = callback;
    }
        
    bool DeviceStreamingImagePool::SetMemoryBudget(size_t newBudget)
    {
        if (newBudget != 0 && newBudget < ImagePoolMininumSizeInBytes)
        {
            return false;
        }

        return SetMemoryBudgetInternal(newBudget) == ResultCode::Success;
    }

    bool DeviceStreamingImagePool::SupportTiledImage() const
    {
        return SupportTiledImageInternal();
    }

    ResultCode DeviceStreamingImagePool::InitInternal(Device&, const StreamingImagePoolDescriptor&)
    {
        return ResultCode::Success;
    }

    ResultCode DeviceStreamingImagePool::InitImageInternal(const DeviceStreamingImageInitRequest&)
    {
        return ResultCode::Unimplemented;
    }

    ResultCode DeviceStreamingImagePool::ExpandImageInternal(const DeviceStreamingImageExpandRequest&)
    {
        return ResultCode::Unimplemented;
    }

    ResultCode DeviceStreamingImagePool::TrimImageInternal(DeviceImage&, uint32_t)
    {
        return ResultCode::Unimplemented;
    }
        
    ResultCode DeviceStreamingImagePool::SetMemoryBudgetInternal([[maybe_unused]] size_t newBudget)
    {
        return ResultCode::Unimplemented;
    }

    bool DeviceStreamingImagePool::SupportTiledImageInternal() const
    {
        return false;
    }
}

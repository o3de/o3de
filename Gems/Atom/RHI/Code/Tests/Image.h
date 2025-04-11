/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI/DeviceImageView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ImageView
        : public AZ::RHI::DeviceImageView
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::DeviceResource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class Image
        : public AZ::RHI::DeviceImage
    {
    public:
        AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);

    private:
        void GetSubresourceLayoutsInternal(const AZ::RHI::ImageSubresourceRange&, AZ::RHI::DeviceImageSubresourceLayout*, size_t*) const override {}
    };

    class ImagePool
        : public AZ::RHI::DeviceImagePool
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::DeviceImageInitRequest& request) override;

        AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::DeviceImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }

        void ShutdownResourceInternal(AZ::RHI::DeviceResource& image) override;
    };
}

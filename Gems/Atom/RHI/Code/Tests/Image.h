/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceImagePool.h>
#include <Atom/RHI/SingleDeviceImageView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ImageView
        : public AZ::RHI::SingleDeviceImageView
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::SingleDeviceResource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class Image
        : public AZ::RHI::SingleDeviceImage
    {
    public:
        AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);

    private:
        void GetSubresourceLayoutsInternal(const AZ::RHI::ImageSubresourceRange&, AZ::RHI::SingleDeviceImageSubresourceLayout*, size_t*) const override {}
    };

    class ImagePool
        : public AZ::RHI::SingleDeviceImagePool
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::SingleDeviceImageInitRequest& request) override;

        AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::SingleDeviceImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }

        void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource& image) override;
    };
}

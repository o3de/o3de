/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class ImageView
        : public AZ::RHI::ImageView
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::Resource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class Image
        : public AZ::RHI::Image
    {
    public:
        AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator, 0);

    private:
        void GetSubresourceLayoutsInternal(const AZ::RHI::ImageSubresourceRange&, AZ::RHI::ImageSubresourceLayoutPlaced*, size_t*) const override {}
    };

    class ImagePool
        : public AZ::RHI::ImagePool
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::ImagePoolDescriptor&) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitImageInternal(const AZ::RHI::ImageInitRequest& request) override;

        AZ::RHI::ResultCode UpdateImageContentsInternal(const AZ::RHI::ImageUpdateRequest&) override { return AZ::RHI::ResultCode::Success; }

        void ShutdownResourceInternal(AZ::RHI::Resource& image) override;
    };
}

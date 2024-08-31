/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImageView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace WebGPU
    {
        class Image;

        class ImageView final
            : public RHI::DeviceImageView
        {
            using Base = RHI::DeviceImageView;
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator);
            AZ_RTTI(ImageView, "{AB053773-4C5E-40C3-A6C0-7990140FE116}", Base);

            static RHI::Ptr<ImageView> Create();

        private:
            ImageView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImageView
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::DeviceResource& resourceBase) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InvalidateInternal() override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

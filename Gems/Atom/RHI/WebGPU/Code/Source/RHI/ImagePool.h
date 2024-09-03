/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImagePool.h>

namespace AZ
{
    namespace WebGPU
    {
        class ImagePool final
            : public RHI::DeviceImagePool
        {
            using Base = RHI::DeviceImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);
            AZ_RTTI(ImagePool, "{26CC4BCD-2239-4935-945E-7B1F568839F3}", RHI::DeviceImagePool);
            
            static RHI::Ptr<ImagePool> Create();
            
        private:
            ImagePool() = default;
                        
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImagePool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device&, [[maybe_unused]] const RHI::ImagePoolDescriptor&) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::DeviceImageInitRequest& request) override { return RHI::ResultCode::Success;}
            RHI::ResultCode UpdateImageContentsInternal([[maybe_unused]] const RHI::DeviceImageUpdateRequest& request) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::DeviceResource& resourceBase) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

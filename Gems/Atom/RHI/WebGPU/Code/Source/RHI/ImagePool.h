/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImagePool.h>

namespace AZ::WebGPU
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
        RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
        RHI::ResultCode InitImageInternal(const RHI::DeviceImageInitRequest& request) override;
        RHI::ResultCode UpdateImageContentsInternal(const RHI::DeviceImageUpdateRequest& request) override;
        void ShutdownInternal() override;
        void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
        //////////////////////////////////////////////////////////////////////////
    };
}

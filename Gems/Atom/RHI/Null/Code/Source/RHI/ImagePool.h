/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceImagePool.h>

namespace AZ
{
    namespace Null
    {
        class ImagePool final
            : public RHI::SingleDeviceImagePool
        {
            using Base = RHI::SingleDeviceImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);
            AZ_RTTI(ImagePool, "{40DC04EA-890C-4D99-9F9C-DEC0038D7451}", RHI::SingleDeviceImagePool);
            
            static RHI::Ptr<ImagePool> Create();
            
        private:
            ImagePool() = default;
                        
            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceImagePool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device&, [[maybe_unused]] const RHI::ImagePoolDescriptor&) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::SingleDeviceImageInitRequest& request) override { return RHI::ResultCode::Success;}
            RHI::ResultCode UpdateImageContentsInternal([[maybe_unused]] const RHI::SingleDeviceImageUpdateRequest& request) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::SingleDeviceResource& resourceBase) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

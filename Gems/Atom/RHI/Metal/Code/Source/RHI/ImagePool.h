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
    namespace Metal
    {
        class ImagePoolResolver;
    
        class Device;
        class ImagePool final
            : public RHI::SingleDeviceImagePool
        {
            using Base = RHI::SingleDeviceImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);
            AZ_RTTI(ImagePool, "{04E85806-9E84-4BD0-99F6-EEBA32B1C4F7}", RHI::SingleDeviceImagePool);
            
            static RHI::Ptr<ImagePool> Create();
            
            Device& GetDevice() const;
            ImagePoolResolver* GetResolver();
        private:
            ImagePool() = default;
            
            void AddMemoryUsage(size_t bytesToAdd);
            void SubtractMemoryUsage(size_t bytesToSubtract);
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceImagePool
            RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
            RHI::ResultCode InitImageInternal(const RHI::SingleDeviceImageInitRequest& request) override;
            RHI::ResultCode UpdateImageContentsInternal(const RHI::SingleDeviceImageUpdateRequest& request) override;
            void ShutdownResourceInternal(RHI::SingleDeviceResource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

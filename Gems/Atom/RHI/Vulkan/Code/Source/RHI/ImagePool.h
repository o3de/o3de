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
    namespace Vulkan
    {
        class Device;

        class ImagePool final
            : public RHI::SingleDeviceImagePool
        {
            using Base = RHI::SingleDeviceImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);
            AZ_RTTI(ImagePool, "35351DF3-823C-4042-A8BA-D6FE10FF6A8D", Base);

            static RHI::Ptr<ImagePool> Create();

        private:
            ImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceImagePool
            RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
            RHI::ResultCode InitImageInternal(const RHI::SingleDeviceImageInitRequest& request) override;
            RHI::ResultCode UpdateImageContentsInternal(const RHI::SingleDeviceImageUpdateRequest& request) override;
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::SingleDeviceResource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

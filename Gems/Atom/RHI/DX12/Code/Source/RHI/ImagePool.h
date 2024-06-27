/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/DeviceImagePool.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class ImagePoolResolver;

        class ImagePool final
            : public RHI::DeviceImagePool
        {
            using Base = RHI::DeviceImagePool;
        public:
            AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator);
            AZ_RTTI(ImagePool, "{084A02C0-DBCB-4285-B79E-842B49292B5E}", RHI::DeviceImagePool);

            static RHI::Ptr<ImagePool> Create();

            Device& GetDevice() const;

        private:
            ImagePool() = default;

            ImagePoolResolver* GetResolver();

            void AddMemoryUsage(size_t bytesToAdd);
            void SubtractMemoryUsage(size_t bytesToSubtract);

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImagePool
            RHI::ResultCode InitInternal(RHI::Device&, const RHI::ImagePoolDescriptor&) override;
            RHI::ResultCode InitImageInternal(const RHI::DeviceImageInitRequest& request) override;
            RHI::ResultCode UpdateImageContentsInternal(const RHI::DeviceImageUpdateRequest& request) override;
            void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

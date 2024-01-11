/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceBufferPool.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class BufferPoolResolver;
        class Device;

        class BufferPool final
            : public RHI::SingleDeviceBufferPool
        {
            using Base = RHI::SingleDeviceBufferPool;

        public:
            AZ_RTTI(BufferPool, "F3DE9E13-12F2-489E-8665-6895FD7446C0", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);
            ~BufferPool() = default;
            static RHI::Ptr<BufferPool> Create();

            Device& GetDevice() const;

        private:
            BufferPool() = default;

            BufferPoolResolver* GetResolver();

            //////////////////////////////////////////////////////////////////////////
             // RHI::SingleDeviceBufferPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::BufferPoolDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode InitBufferInternal(RHI::SingleDeviceBuffer& buffer, const RHI::BufferDescriptor& rhiDescriptor) override;
            void ShutdownResourceInternal(RHI::SingleDeviceResource& resource) override;
            RHI::ResultCode OrphanBufferInternal(RHI::SingleDeviceBuffer& buffer) override;
            RHI::ResultCode MapBufferInternal(const RHI::SingleDeviceBufferMapRequest& mapRequest, RHI::SingleDeviceBufferMapResponse& response) override;
            void UnmapBufferInternal(RHI::SingleDeviceBuffer& buffer) override;
            RHI::ResultCode StreamBufferInternal(const RHI::SingleDeviceBufferStreamRequest& request) override;
            void ComputeFragmentation() const override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

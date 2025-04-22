/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBufferPool.h>

namespace AZ
{
    namespace Null
    {
        class BufferPool final
            : public RHI::DeviceBufferPool
        {
            using Base = RHI::DeviceBufferPool;
        public:
            AZ_RTTI(BufferPool, "{5F02ECA6-135C-4C96-895E-514E273EEFC2}", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);
            virtual ~BufferPool() = default;
            
            static RHI::Ptr<BufferPool> Create();
            
        private:
            BufferPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceBufferPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::BufferPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            RHI::ResultCode InitBufferInternal([[maybe_unused]] RHI::DeviceBuffer& buffer, [[maybe_unused]] const RHI::BufferDescriptor& rhiDescriptor) override{ return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::DeviceResource& resource) override {}
            RHI::ResultCode OrphanBufferInternal([[maybe_unused]] RHI::DeviceBuffer& buffer) override { return RHI::ResultCode::Success;}
            RHI::ResultCode MapBufferInternal([[maybe_unused]] const RHI::DeviceBufferMapRequest& mapRequest, [[maybe_unused]] RHI::DeviceBufferMapResponse& response) override { return RHI::ResultCode::Success;}
            void UnmapBufferInternal([[maybe_unused]] RHI::DeviceBuffer& buffer) override {}
            RHI::ResultCode StreamBufferInternal([[maybe_unused]] const RHI::DeviceBufferStreamRequest& request) override { return RHI::ResultCode::Success;}
            void BufferCopy([[maybe_unused]] void* destination, [[maybe_unused]] const void* source, [[maybe_unused]] size_t num) override {}
            void ComputeFragmentation() const override {}
            //////////////////////////////////////////////////////////////////////////

        };

    }
}

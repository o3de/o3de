/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBufferPool.h>
#include <RHI/BufferMemoryAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class BufferPoolResolver;

        class BufferPool final
            : public RHI::DeviceBufferPool
        {
            using Base = RHI::DeviceBufferPool;
        public:
            AZ_RTTI(BufferPool, "{A0912F67-86AB-47B1-B764-793C939306E7}", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);
            virtual ~BufferPool() = default;
            
            static RHI::Ptr<BufferPool> Create();
            
        private:
            BufferPool() = default;

            Device& GetDevice() const;
            
            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceBufferPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::BufferPoolDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode InitBufferInternal(RHI::DeviceBuffer& buffer, const RHI::BufferDescriptor& rhiDescriptor) override;
            void ShutdownResourceInternal(RHI::DeviceResource& resource) override;
            RHI::ResultCode OrphanBufferInternal(RHI::DeviceBuffer& buffer) override;
            RHI::ResultCode MapBufferInternal(const RHI::DeviceBufferMapRequest& mapRequest, RHI::DeviceBufferMapResponse& response) override;
            void UnmapBufferInternal(RHI::DeviceBuffer& buffer) override;
            RHI::ResultCode StreamBufferInternal(const RHI::DeviceBufferStreamRequest& request) override;
            void ComputeFragmentation() const override;
            //////////////////////////////////////////////////////////////////////////
            
            BufferPoolResolver* GetResolver();
            
            BufferMemoryAllocator m_allocator;
        };

    }
}

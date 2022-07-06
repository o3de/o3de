/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Vulkan/BufferPoolDescriptor.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <RHI/BufferMemoryAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class BufferPoolResolver;
        class Device;

        class BufferPool final
            : public RHI::DeviceBufferPool
        {
            using Base = RHI::DeviceBufferPool;

        public:
            AZ_RTTI(BufferPool, "F3DE9E13-12F2-489E-8665-6895FD7446C0", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);
            ~BufferPool() = default;
            static RHI::Ptr<BufferPool> Create();

            Device& GetDevice() const;

            void GarbageCollect();

        private:
            BufferPool() = default;

            BufferPoolResolver* GetResolver();

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
             // RHI::BufferPool
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

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            BufferMemoryAllocator m_memoryAllocator;
        };
    }
}

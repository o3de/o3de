/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceBufferPool.h>
#include <RHI/Device.h>

#ifdef USE_AMD_D3D12MA
#include <RHI/BufferD3D12MemoryAllocator.h>
#endif
#include <RHI/BufferMemoryAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class BufferPoolResolver;

        class BufferPool final
            : public RHI::SingleDeviceBufferPool
        {
            using Base = RHI::SingleDeviceBufferPool;
        public:
            AZ_RTTI(BufferPool, "{BC251841-AADD-4A4A-A4FF-4F94897541D5}", Base);
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

            BufferPoolResolver* GetResolver();

#ifdef USE_AMD_D3D12MA
            BufferD3D12MemoryAllocator m_allocator;
#else
            BufferMemoryAllocator m_allocator;
#endif
        };
    }
}

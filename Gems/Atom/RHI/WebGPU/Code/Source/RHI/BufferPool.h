/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBufferPool.h>

namespace AZ::WebGPU
{
    //! WebGPU BufferPoolDescriptor for custom flags
    struct BufferPoolDescriptor : public RHI::BufferPoolDescriptor
    {
    public:
        AZ_RTTI(BufferPoolDescriptor, "{12E84CA4-1D88-4E72-9987-C75CA1E2D61F}", RHI::BufferPoolDescriptor);
        BufferPoolDescriptor() = default;

        //! Map the buffer during initialization
        bool m_mappedAtCreation = false;
    };

    class BufferPool final
        : public RHI::DeviceBufferPool
    {
        using Base = RHI::DeviceBufferPool;
    public:
        AZ_RTTI(BufferPool, "{CFFC66EA-DBF9-4A6B-BBC4-499208AE08E0}", Base);
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
        RHI::ResultCode InitInternal(RHI::Device& device, const RHI::BufferPoolDescriptor& descriptor) override;
        void ShutdownInternal() override;
        RHI::ResultCode InitBufferInternal(RHI::DeviceBuffer& buffer, const RHI::BufferDescriptor& rhiDescriptor) override;
        void ShutdownResourceInternal(RHI::DeviceResource& resource) override;
        RHI::ResultCode OrphanBufferInternal(RHI::DeviceBuffer& buffer) override;
        RHI::ResultCode MapBufferInternal(
            const RHI::DeviceBufferMapRequest& mapRequest, RHI::DeviceBufferMapResponse& response) override;
        void UnmapBufferInternal(RHI::DeviceBuffer& buffer) override;
        RHI::ResultCode StreamBufferInternal([[maybe_unused]] const RHI::DeviceBufferStreamRequest& request) override { return RHI::ResultCode::Success;}
        void ComputeFragmentation() const override {}
        //////////////////////////////////////////////////////////////////////////

        //! Extra init flags to use when initializing buffers
        Buffer::InitFlags m_extraInitFlags = Buffer::InitFlags::None;
    };
}

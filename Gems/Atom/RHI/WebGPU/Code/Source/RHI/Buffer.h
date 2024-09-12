/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AsyncWorkQueue.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::WebGPU
{
    class Device;

    class Buffer final
        : public RHI::DeviceBuffer
    {
        using Base = RHI::DeviceBuffer;
        friend class BufferPool;
        friend class AliasedHeap;

    public:
        AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator);
        AZ_RTTI(Buffer, "{8C858CF3-E360-42EC-A6FF-D441F60D7D01}", Base);
        ~Buffer() = default;

        enum class InitFlags : uint8_t
        {
            None = 0,
            MapRead = AZ_BIT(0),
            MapWrite = AZ_BIT(1),
            MapReadWrite = MapRead | MapWrite,
            MappedAtCreation = AZ_BIT(2)
        };
            
        static RHI::Ptr<Buffer> Create();
        const wgpu::Buffer& GetNativeBuffer() const;
        bool CanBeMap() const;

        void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
        const RHI::AsyncWorkHandle& GetUploadHandle() const;

    private:

        Buffer() = default;
        RHI::ResultCode Init(Device& device, const RHI::BufferDescriptor& bufferDescriptor, InitFlags initFlags = InitFlags::None);

        void Invalidate();

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceResource
        void ReportMemoryUsage([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) const override {}
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //! Native buffer
        wgpu::Buffer m_wgpuBuffer = nullptr;
        wgpu::BufferUsage m_wgpuBufferUsage = wgpu::BufferUsage::None;

        RHI::AsyncWorkHandle m_uploadHandle;
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::WebGPU::Buffer::InitFlags);
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/DX12.h>
#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;

        //! This class builds and contains the DX12 RayTracing TLAS buffers.
        class RayTracingTlas final
            : public RHI::SingleDeviceRayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingTlas> Create();

            struct TlasBuffers
            {
                RHI::Ptr<RHI::SingleDeviceBuffer> m_tlasBuffer;
                RHI::Ptr<RHI::SingleDeviceBuffer> m_scratchBuffer;
                RHI::Ptr<RHI::SingleDeviceBuffer> m_tlasInstancesBuffer;
            };

#ifdef AZ_DX12_DXR_SUPPORT
            const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& GetInputs() const { return m_inputs; }
#endif
            const TlasBuffers& GetBuffers() const { return m_buffers[m_currentBufferIndex]; }

            // RHI::SingleDeviceRayTracingTlas overrides...
            const RHI::Ptr<RHI::SingleDeviceBuffer> GetTlasBuffer() const override { return m_buffers[m_currentBufferIndex].m_tlasBuffer; }
            const RHI::Ptr<RHI::SingleDeviceBuffer> GetTlasInstancesBuffer() const override { return m_buffers[m_currentBufferIndex].m_tlasInstancesBuffer; }

        private:
            RayTracingTlas() = default;

            // RHI::SingleDeviceRayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::SingleDeviceRayTracingTlasDescriptor* descriptor, const RHI::SingleDeviceRayTracingBufferPools& rayTracingBufferPools) override;

#ifdef AZ_DX12_DXR_SUPPORT
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS m_inputs;
#endif

            // buffer list to keep buffers alive for several frames
            static const uint32_t BufferCount = AZ::RHI::Limits::Device::FrameCountMax;
            TlasBuffers m_buffers[BufferCount];
            uint32_t m_currentBufferIndex = 0;
        };
    }
}

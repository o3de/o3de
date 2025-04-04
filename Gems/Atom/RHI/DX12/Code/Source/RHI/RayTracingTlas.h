/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/DX12.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;

        //! This class builds and contains the DX12 RayTracing TLAS buffers.
        class RayTracingTlas final
            : public RHI::DeviceRayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingTlas> Create();

            struct TlasBuffers
            {
                RHI::Ptr<RHI::DeviceBuffer> m_tlasBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_scratchBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_tlasInstancesBuffer;
            };

#ifdef AZ_DX12_DXR_SUPPORT
            const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& GetInputs() const { return m_inputs; }
#endif
            const TlasBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

            // RHI::DeviceRayTracingTlas overrides...
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasBuffer() const override { return GetBuffers().m_tlasBuffer; }
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasInstancesBuffer() const override { return GetBuffers().m_tlasInstancesBuffer; }

        private:
            RayTracingTlas() = default;

            // RHI::DeviceRayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingTlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;

#ifdef AZ_DX12_DXR_SUPPORT
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS m_inputs;
#endif

            // buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<TlasBuffers> m_buffers;
        };
    }
}

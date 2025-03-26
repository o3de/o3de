/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;

        //! This class builds and contains the DX12 RayTracing BLAS buffers.
        class RayTracingBlas final
            : public RHI::DeviceRayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingBlas> Create();

            struct BlasBuffers
            {
                RHI::Ptr<RHI::DeviceBuffer> m_blasBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_scratchBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_aabbBuffer;
            };

#ifdef AZ_DX12_DXR_SUPPORT
            const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& GetInputs() const { return m_inputs; }
#endif
            const BlasBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

            // RHI::DeviceRayTracingBlas overrides...
            virtual bool IsValid() const override { return GetBuffers().m_blasBuffer != nullptr; }

            uint64_t GetAccelerationStructureByteSize() override;

        private:
            RayTracingBlas() = default;

            // RHI::DeviceRayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingBlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;
            RHI::ResultCode CreateCompactedBuffersInternal(
                RHI::Device& device,
                RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
                uint64_t compactedBufferSize,
                const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;

#ifdef AZ_DX12_DXR_SUPPORT
            AZStd::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS m_inputs;

            static D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetAccelerationStructureBuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags);
#endif

            // buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<BlasBuffers> m_buffers;
        };
    }
}

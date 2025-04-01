/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class RayTracingAccelerationStructure;

        //! This class builds and contains the Vulkan RayTracing BLAS buffers.
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
                RHI::Ptr<RayTracingAccelerationStructure> m_accelerationStructure;

                AZStd::vector<VkAccelerationStructureGeometryKHR> m_geometryDescs;
                AZStd::vector<VkAccelerationStructureBuildRangeInfoKHR> m_rangeInfos;
                VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo = {};
            };

            const BlasBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

            // RHI::DeviceRayTracingBlas overrides...
            virtual bool IsValid() const override { return GetBuffers().m_accelerationStructure != VK_NULL_HANDLE; }

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

            static VkBuildAccelerationStructureFlagsKHR GetAccelerationStructureBuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags);

            // buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<BlasBuffers> m_buffers;
        };
    }
}

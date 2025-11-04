/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class RayTracingAccelerationStructure;

        //! This class builds and contains the Vulkan RayTracing TLAS buffers.
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
                RHI::Ptr<RayTracingAccelerationStructure> m_accelerationStructure;

                VkAccelerationStructureGeometryKHR m_geometry = {};
                VkAccelerationStructureBuildRangeInfoKHR m_offsetInfo = {};
                VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo = {};
                uint32_t m_instanceCount = 0;
            };

            const TlasBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

            // RHI::DeviceRayTracingTlas overrides...
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasBuffer() const override { return GetBuffers().m_tlasBuffer; }
            const RHI::Ptr<RHI::DeviceBuffer> GetTlasInstancesBuffer() const override { return GetBuffers().m_tlasInstancesBuffer; }

        private:
            RayTracingTlas() = default;

            // RHI::DeviceRayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingTlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;

            // buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<TlasBuffers> m_buffers;
        };
    }
}

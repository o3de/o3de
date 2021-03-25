/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;

        //! This class builds and contains the Vulkan RayTracing BLAS buffers.
        class RayTracingBlas final
            : public RHI::RayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingBlas> Create();

            struct BlasBuffers
            {
                RHI::Ptr<RHI::Buffer> m_blasBuffer;
                RHI::Ptr<RHI::Buffer> m_scratchBuffer;
                VkAccelerationStructureKHR m_accelerationStructure = VK_NULL_HANDLE;

                AZStd::vector<VkAccelerationStructureGeometryKHR> m_geometryDescs;
                AZStd::vector<VkAccelerationStructureBuildRangeInfoKHR> m_rangeInfos;
                VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo = {};
            };

            const BlasBuffers& GetBuffers() const { return m_buffers[m_currentBufferIndex]; }
 
            // RHI::RayTracingBlas overrides...
            virtual bool IsValid() const override { return m_buffers[m_currentBufferIndex].m_accelerationStructure != VK_NULL_HANDLE; }

        private:
            RayTracingBlas() = default;

            // RHI::RayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::RayTracingBlasDescriptor* descriptor, const RHI::RayTracingBufferPools& rayTracingBufferPools) override;

            // buffer list to keep buffers alive for several frames
            static const uint32_t BufferCount = 3;
            BlasBuffers m_buffers[BufferCount];
            uint32_t m_currentBufferIndex = 0;
        };
    }
}

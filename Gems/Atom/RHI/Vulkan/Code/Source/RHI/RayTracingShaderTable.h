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

#include <Atom/RHI/RayTracingShaderTable.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/Buffer.h>

namespace AZ
{
    namespace Vulkan
    {
        class RayTracingShaderTable final
            : public RHI::RayTracingShaderTable
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingShaderTable> Create();

            struct ShaderTableBuffers
            {
                RHI::Ptr<RHI::Buffer> m_rayGenerationTable;
                uint32_t m_rayGenerationTableSize = 0;
                uint32_t m_rayGenerationTableStride = 0;

                RHI::Ptr<RHI::Buffer> m_missTable;
                uint32_t m_missTableSize = 0;
                uint32_t m_missTableStride = 0;

                RHI::Ptr<RHI::Buffer> m_hitGroupTable;
                uint32_t m_hitGroupTableSize = 0;
                uint32_t m_hitGroupTableStride = 0;
            };

            const ShaderTableBuffers& GetBuffers() const { return m_buffers[m_currentBufferIndex]; }

        private:

            RayTracingShaderTable() = default;

            RHI::Ptr<RHI::Buffer> BuildTable(
                const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties,
                const RayTracingPipelineState* rayTracingPipelineState,
                const RHI::RayTracingBufferPools& bufferPools,
                const RHI::RayTracingShaderTableRecordList& recordList,
                uint32_t shaderRecordSize,
                AZStd::string shaderTableName);

            //////////////////////////////////////////////////////////////////////////
            // RHI::RayTracingShaderTable
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::RayTracingShaderTableDescriptor* descriptor, const RHI::RayTracingBufferPools& bufferPools) override;
            //////////////////////////////////////////////////////////////////////////

            static const uint32_t BufferCount = 3;
            ShaderTableBuffers m_buffers[BufferCount];
            uint32_t m_currentBufferIndex = 0;
        };
    }
}

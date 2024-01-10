/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceRayTracingShaderTable.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/SingleDeviceBuffer.h>

namespace AZ
{
    namespace Vulkan
    {
        class RayTracingShaderTable final
            : public RHI::SingleDeviceRayTracingShaderTable
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingShaderTable> Create();

            struct ShaderTableBuffers
            {
                RHI::Ptr<RHI::SingleDeviceBuffer> m_rayGenerationTable;
                uint32_t m_rayGenerationTableSize = 0;
                uint32_t m_rayGenerationTableStride = 0;

                RHI::Ptr<RHI::SingleDeviceBuffer> m_missTable;
                uint32_t m_missTableSize = 0;
                uint32_t m_missTableStride = 0;

                RHI::Ptr<RHI::SingleDeviceBuffer> m_callableTable;
                uint32_t m_callableTableSize = 0;
                uint32_t m_callableTableStride = 0;

                RHI::Ptr<RHI::SingleDeviceBuffer> m_hitGroupTable;
                uint32_t m_hitGroupTableSize = 0;
                uint32_t m_hitGroupTableStride = 0;
            };

            const ShaderTableBuffers& GetBuffers() const { return m_buffers[m_currentBufferIndex]; }

        private:

            RayTracingShaderTable() = default;

            RHI::Ptr<RHI::SingleDeviceBuffer> BuildTable(
                const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties,
                const RayTracingPipelineState* rayTracingPipelineState,
                const RHI::SingleDeviceRayTracingBufferPools& bufferPools,
                const RHI::RayTracingShaderTableRecordList& recordList,
                uint32_t shaderRecordSize,
                AZStd::string shaderTableName);

            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceRayTracingShaderTable
            RHI::ResultCode BuildInternal() override;
            //////////////////////////////////////////////////////////////////////////

            static const uint32_t BufferCount = 3;
            ShaderTableBuffers m_buffers[BufferCount];
            uint32_t m_currentBufferIndex = 0;
        };
    }
}

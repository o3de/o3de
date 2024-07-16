/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/DeviceBuffer.h>

namespace AZ
{
    namespace Vulkan
    {
        class RayTracingShaderTable final
            : public RHI::DeviceRayTracingShaderTable
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingShaderTable> Create();

            struct ShaderTableBuffers
            {
                RHI::Ptr<RHI::DeviceBuffer> m_rayGenerationTable;
                uint32_t m_rayGenerationTableSize = 0;
                uint32_t m_rayGenerationTableStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_missTable;
                uint32_t m_missTableSize = 0;
                uint32_t m_missTableStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_callableTable;
                uint32_t m_callableTableSize = 0;
                uint32_t m_callableTableStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_hitGroupTable;
                uint32_t m_hitGroupTableSize = 0;
                uint32_t m_hitGroupTableStride = 0;
            };

            const ShaderTableBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

        private:

            RayTracingShaderTable() = default;

            RHI::Ptr<RHI::DeviceBuffer> BuildTable(
                const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties,
                const RayTracingPipelineState* rayTracingPipelineState,
                const RHI::DeviceRayTracingBufferPools& bufferPools,
                const RHI::DeviceRayTracingShaderTableRecordList& recordList,
                uint32_t shaderRecordSize,
                AZStd::string shaderTableName);

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingShaderTable
            RHI::ResultCode BuildInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::FrameCountMaxRingBuffer<ShaderTableBuffers> m_buffers;
        };
    }
}

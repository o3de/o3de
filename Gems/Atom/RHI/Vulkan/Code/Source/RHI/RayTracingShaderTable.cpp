/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Debug/EventTrace.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingShaderTable> RayTracingShaderTable::Create()
        {
            return aznew RayTracingShaderTable;
        }

        RHI::Ptr<RHI::Buffer> RayTracingShaderTable::BuildTable(
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties,
            const RayTracingPipelineState* rayTracingPipelineState,
            const RHI::RayTracingBufferPools& bufferPools,
            const RHI::RayTracingShaderTableRecordList& recordList,
            uint32_t shaderRecordSize,
            AZStd::string shaderTableName)
        {
            uint32_t shaderTableSize = shaderRecordSize * static_cast<uint32_t>(recordList.size());

            RHI::Ptr<RHI::Buffer> shaderTableBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor shaderTableBufferDescriptor;
            shaderTableBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingShaderTable;
            shaderTableBufferDescriptor.m_byteCount = shaderTableSize;

            AZ::RHI::BufferInitRequest shaderTableBufferRequest;
            shaderTableBufferRequest.m_buffer = shaderTableBuffer.get();
            shaderTableBufferRequest.m_descriptor = shaderTableBufferDescriptor;
            RHI::ResultCode resultCode = bufferPools.GetShaderTableBufferPool()->InitBuffer(shaderTableBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create shader table buffer");

            BufferMemoryView* shaderTableMemoryView = static_cast<Buffer*>(shaderTableBuffer.get())->GetBufferMemoryView();
            shaderTableMemoryView->SetName(shaderTableName);

            RHI::BufferMapResponse mapResponse;
            resultCode = bufferPools.GetShaderTableBufferPool()->MapBuffer(RHI::BufferMapRequest(*shaderTableBuffer, 0, shaderTableSize), mapResponse);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to map shader table buffer");
            uint8_t* mappedData = reinterpret_cast<uint8_t*>(mapResponse.m_data);

            for (auto& record : recordList)
            {
                uint8_t* shaderHandle = rayTracingPipelineState->GetShaderHandle(record.m_shaderExportName);
                AZ_Assert(shaderHandle, "Failed to retrieve HitGroup shader handle");

                memcpy(mappedData, shaderHandle, rayTracingPipelineProperties.shaderGroupHandleSize);
                mappedData += rayTracingPipelineProperties.shaderGroupHandleSize;
                mappedData = RHI::AlignUp(mappedData, rayTracingPipelineProperties.shaderGroupBaseAlignment);
            }

            bufferPools.GetShaderTableBufferPool()->UnmapBuffer(*shaderTableBuffer);

            return static_cast<Buffer*>(shaderTableBuffer.get());
        }

        RHI::ResultCode RayTracingShaderTable::BuildInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = physicalDevice.GetPhysicalDeviceRayTracingPipelineProperties();
            uint32_t shaderHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
            uint32_t alignedShaderHandleSize = RHI::AlignUp(shaderHandleSize, rayTracingPipelineProperties.shaderGroupBaseAlignment);

            // advance to the next buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
            ShaderTableBuffers& buffers = m_buffers[m_currentBufferIndex];

            // clear the shader table if the descriptor has no ray generation shader
            if (m_descriptor->GetRayGenerationRecord().empty())
            {
                buffers.m_rayGenerationTable = nullptr;
                buffers.m_rayGenerationTableStride = 0;
                buffers.m_rayGenerationTableSize = 0;
                buffers.m_missTable = nullptr;
                buffers.m_missTableSize = 0;
                buffers.m_missTableStride = 0;
                buffers.m_hitGroupTable = nullptr;
                buffers.m_hitGroupTableSize = 0;
                buffers.m_hitGroupTableStride = 0;
                return RHI::ResultCode::Success;
            }

            // calculate record sizes
            buffers.m_rayGenerationTableStride = RHI::AlignUp(alignedShaderHandleSize, rayTracingPipelineProperties.shaderGroupBaseAlignment);
            buffers.m_missTableStride = RHI::AlignUp(alignedShaderHandleSize, rayTracingPipelineProperties.shaderGroupBaseAlignment);
            buffers.m_hitGroupTableStride = RHI::AlignUp(alignedShaderHandleSize, rayTracingPipelineProperties.shaderGroupBaseAlignment);

            // calculate sub-table sizes
            buffers.m_rayGenerationTableSize = buffers.m_rayGenerationTableStride * aznumeric_cast<uint32_t>(m_descriptor->GetRayGenerationRecord().size());
            buffers.m_missTableSize = buffers.m_missTableStride * aznumeric_cast<uint32_t>(m_descriptor->GetMissRecords().size());
            buffers.m_hitGroupTableSize = buffers.m_hitGroupTableStride * aznumeric_cast<uint32_t>(m_descriptor->GetHitGroupRecords().size());

            const RayTracingPipelineState* rayTracingPipelineState = static_cast<const RayTracingPipelineState*>(m_descriptor->GetPipelineState().get());

            // build sub-tables
            buffers.m_rayGenerationTable = BuildTable(
                rayTracingPipelineProperties,
                rayTracingPipelineState,
                *m_bufferPools,
                m_descriptor->GetRayGenerationRecord(),
                buffers.m_rayGenerationTableStride,
                "RayGenerationTable");

            buffers.m_missTable = BuildTable(
                rayTracingPipelineProperties,
                rayTracingPipelineState,
                *m_bufferPools,
                m_descriptor->GetMissRecords(),
                buffers.m_missTableStride,
                "MissTable");

            buffers.m_hitGroupTable = BuildTable(
                rayTracingPipelineProperties,
                rayTracingPipelineState,
                *m_bufferPools,
                m_descriptor->GetHitGroupRecords(),
                buffers.m_hitGroupTableStride,
                "HitGroupTable");

            return RHI::ResultCode::Success;
        }
    }
}

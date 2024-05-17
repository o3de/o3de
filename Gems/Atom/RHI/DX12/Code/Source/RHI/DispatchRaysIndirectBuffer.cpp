/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DispatchRaysIndirectBuffer.h"

#include <RHI/Buffer.h>
#include <RHI/DX12.h>
#include <RHI/Device.h>
#include <RHI/RayTracingShaderTable.h>

#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<DispatchRaysIndirectBuffer> DispatchRaysIndirectBuffer::Create()
        {
            return aznew DispatchRaysIndirectBuffer;
        }

        void DispatchRaysIndirectBuffer::Init(RHI::DeviceBufferPool* bufferPool)
        {
            m_buffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = bufferPool->GetDescriptor().m_bindFlags;
            bufferDescriptor.m_byteCount = sizeof(D3D12_DISPATCH_RAYS_DESC);

            AZ::RHI::DeviceBufferInitRequest bufferRequest;
            bufferRequest.m_buffer = m_buffer.get();
            bufferRequest.m_descriptor = bufferDescriptor;
            [[maybe_unused]] RHI::ResultCode resultCode = bufferPool->InitBuffer(bufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create DispatchRaysIndirectBuffer buffer");
        }

        void DispatchRaysIndirectBuffer::Build(RHI::DeviceRayTracingShaderTable* shaderTable)
        {
            const RayTracingShaderTable* dxShaderTable = static_cast<const RayTracingShaderTable*>(shaderTable);
            const RayTracingShaderTable::ShaderTableBuffers& shaderTableBuffers = dxShaderTable->GetBuffers();
            D3D12_DISPATCH_RAYS_DESC desc = {};
            desc.RayGenerationShaderRecord.StartAddress = shaderTableBuffers.m_rayGenerationTable->GetMemoryView().GetGpuAddress();
            desc.RayGenerationShaderRecord.SizeInBytes = shaderTableBuffers.m_rayGenerationTableSize;

            desc.MissShaderTable.StartAddress = shaderTableBuffers.m_missTable->GetMemoryView().GetGpuAddress();
            desc.MissShaderTable.SizeInBytes = shaderTableBuffers.m_missTableSize;
            desc.MissShaderTable.StrideInBytes = shaderTableBuffers.m_missTableStride;

            desc.HitGroupTable.StartAddress = shaderTableBuffers.m_hitGroupTable->GetMemoryView().GetGpuAddress();
            desc.HitGroupTable.SizeInBytes = shaderTableBuffers.m_hitGroupTableSize;
            desc.HitGroupTable.StrideInBytes = shaderTableBuffers.m_hitGroupTableStride;

            desc.Width = 0;
            desc.Height = 0;
            desc.Depth = 0;

            // Acquire staging buffer, and copy data to it
            m_shaderTableStagingMemory =
                static_cast<Device*>(&m_buffer->GetDevice())->AcquireStagingMemory(sizeof(D3D12_DISPATCH_RAYS_DESC), 16);
            auto cpuAddress = m_shaderTableStagingMemory.Map(RHI::HostMemoryAccess::Write);
            AZ_Assert(cpuAddress != nullptr, "Failed to map buffer for IndirectBufferWriter");
            memcpy(cpuAddress, &desc, sizeof(D3D12_DISPATCH_RAYS_DESC));
            m_shaderTableStagingMemory.Unmap(RHI::HostMemoryAccess::Write);
            m_shaderTableNeedsCopy = true;
        }
    } // namespace DX12
} // namespace AZ
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
#include <RHI/RayTracingShaderTable.h>

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingShaderTable.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<DispatchRaysIndirectBuffer> DispatchRaysIndirectBuffer::Create()
        {
            return aznew DispatchRaysIndirectBuffer;
        }

        void DispatchRaysIndirectBuffer::Init(RHI::BufferPool* bufferPool, RHI::BufferPool* stagingBufferPool)
        {
            auto CreateIndirectBuffer = [](RHI::BufferPool* bufferPool)
            {
                auto result = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_bindFlags = bufferPool->GetDescriptor().m_bindFlags;
                bufferDescriptor.m_byteCount = sizeof(D3D12_DISPATCH_RAYS_DESC);

                AZ::RHI::BufferInitRequest bufferRequest;
                bufferRequest.m_buffer = result.get();
                bufferRequest.m_descriptor = bufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode = bufferPool->InitBuffer(bufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create DispatchRaysIndirectBuffer buffer");
                return result;
            };
            m_stagingBuffer = CreateIndirectBuffer(stagingBufferPool);
            m_buffer = CreateIndirectBuffer(bufferPool);
        }

        void DispatchRaysIndirectBuffer::Build(RHI::RayTracingShaderTable* shaderTable)
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

            // Map staging buffer, and copy data to it
            RHI::BufferPool* pool = static_cast<RHI::BufferPool*>(m_stagingBuffer->GetPool());
            RHI::BufferMapRequest request = {};
            RHI::BufferMapResponse response = {};
            request.m_buffer = m_stagingBuffer.get();
            request.m_byteCount = sizeof(D3D12_DISPATCH_RAYS_DESC);
            request.m_byteOffset = 0;
            [[maybe_unused]] RHI::ResultCode result = pool->MapBuffer(request, response);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to map buffer for IndirectBufferWriter");
            memcpy(response.m_data, &desc, sizeof(D3D12_DISPATCH_RAYS_DESC));
            pool->UnmapBuffer(*m_stagingBuffer);
            m_shaderTableNeedsCopy = true;
        }

        void DispatchRaysIndirectBuffer::CopyData(
            const RHI::IndirectBufferView& originalIndirectBufferView,
            uint64_t originalBufferByteOffset,
            ID3D12GraphicsCommandList* commandList)
        {
            const Buffer* dx12Buffer = static_cast<const Buffer*>(m_buffer.get());
            constexpr ptrdiff_t widthOffset = offsetof(D3D12_DISPATCH_RAYS_DESC, Width);

            {
                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = dx12Buffer->GetMemoryView().GetMemory();
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                commandList->ResourceBarrier(1, &barrier);
            }

            if (m_shaderTableNeedsCopy)
            {
                const Buffer* dx12StagingBuffer = static_cast<const Buffer*>(m_stagingBuffer.get());
                m_shaderTableNeedsCopy = false;
                commandList->CopyBufferRegion(
                    dx12Buffer->GetMemoryView().GetMemory(),
                    dx12Buffer->GetMemoryView().GetOffset(),
                    dx12StagingBuffer->GetMemoryView().GetMemory(),
                    dx12StagingBuffer->GetMemoryView().GetOffset(),
                    widthOffset); // copy the shader table entries only
            }
            constexpr ptrdiff_t sizeToCopy = sizeof(uint32_t) * 3;

            const Buffer* dx12OriginalBuffer = static_cast<const Buffer*>(originalIndirectBufferView.GetBuffer());

            {
                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = dx12OriginalBuffer->GetMemoryView().GetMemory();
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                commandList->ResourceBarrier(1, &barrier);
            }

            commandList->CopyBufferRegion(
                dx12Buffer->GetMemoryView().GetMemory(),
                dx12Buffer->GetMemoryView().GetOffset() + widthOffset,
                dx12OriginalBuffer->GetMemoryView().GetMemory(),
                originalIndirectBufferView.GetByteOffset() + originalBufferByteOffset,
                sizeToCopy);

            {
                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = dx12OriginalBuffer->GetMemoryView().GetMemory();
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                commandList->ResourceBarrier(1, &barrier);
            }

            {
                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = dx12Buffer->GetMemoryView().GetMemory();
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                commandList->ResourceBarrier(1, &barrier);
            }
        }
    } // namespace DX12
} // namespace AZ
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/EventTrace.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RayTracingBufferPools.h>


namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingBlas> RayTracingBlas::Create()
        {
            return aznew RayTracingBlas;
        }

        RHI::ResultCode RayTracingBlas::CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& bufferPools)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            Device& device = static_cast<Device&>(deviceBase);
            ID3D12DeviceX* dx12Device = device.GetDevice();

            // advance to the next buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
            BlasBuffers& buffers = m_buffers[m_currentBufferIndex];

            const RHI::RayTracingGeometryVector& geometries = descriptor->GetGeometries();

            // build the list of D3D12_RAYTRACING_GEOMETRY_DESC structures
            m_geometryDescs.clear();
            m_geometryDescs.reserve(geometries.size());
            D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
            for (const RHI::RayTracingGeometry& geometry : geometries)
            {
                geometryDesc = {};
                geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                geometryDesc.Triangles.VertexBuffer.StartAddress = static_cast<const DX12::Buffer*>(geometry.m_vertexBuffer.GetBuffer())->GetMemoryView().GetGpuAddress() + geometry.m_vertexBuffer.GetByteOffset();
                geometryDesc.Triangles.VertexBuffer.StrideInBytes = geometry.m_vertexBuffer.GetByteStride();
                geometryDesc.Triangles.VertexCount = geometry.m_vertexBuffer.GetByteCount() / aznumeric_cast<UINT>(geometryDesc.Triangles.VertexBuffer.StrideInBytes);
                geometryDesc.Triangles.VertexFormat = ConvertFormat(geometry.m_vertexFormat);
                geometryDesc.Triangles.IndexBuffer = static_cast<const DX12::Buffer*>(geometry.m_indexBuffer.GetBuffer())->GetMemoryView().GetGpuAddress() + geometry.m_indexBuffer.GetByteOffset();
                geometryDesc.Triangles.IndexFormat = (geometry.m_indexBuffer.GetIndexFormat() == RHI::IndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                geometryDesc.Triangles.IndexCount = aznumeric_cast<UINT>(geometry.m_indexBuffer.GetByteCount()) / ((geometry.m_indexBuffer.GetIndexFormat() == RHI::IndexFormat::Uint16) ? 2 : 4);
                geometryDesc.Triangles.Transform3x4 = 0; // [GFX-TODO][ATOM-4989] Add DXR BLAS Transform Buffer
                geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                m_geometryDescs.push_back(geometryDesc);
            }

            // retrieve the required sizes for the scratch and result buffers            
            ZeroMemory(&m_inputs, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));
            m_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            m_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            m_inputs.pGeometryDescs = m_geometryDescs.data();
            m_inputs.NumDescs = aznumeric_cast<UINT>(m_geometryDescs.size());
            m_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
            dx12Device->GetRaytracingAccelerationStructurePrebuildInfo(&m_inputs, &prebuildInfo);

            prebuildInfo.ScratchDataSizeInBytes = RHI::AlignUp(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
            prebuildInfo.ResultDataMaxSizeInBytes = RHI::AlignUp(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

            // create scratch buffer
            buffers.m_scratchBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor scratchBufferDescriptor;
            scratchBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            scratchBufferDescriptor.m_byteCount = prebuildInfo.ScratchDataSizeInBytes;

            AZ::RHI::BufferInitRequest scratchBufferRequest;
            scratchBufferRequest.m_buffer = buffers.m_scratchBuffer.get();
            scratchBufferRequest.m_descriptor = scratchBufferDescriptor;
            RHI::ResultCode resultCode = bufferPools.GetScratchBufferPool()->InitBuffer(scratchBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS scratch buffer");

            MemoryView& scratchMemoryView = static_cast<Buffer*>(buffers.m_scratchBuffer.get())->GetMemoryView();
            scratchMemoryView.SetName(L"BLAS Scratch");

            // create BLAS buffer
            buffers.m_blasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor blasBufferDescriptor;
            blasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            blasBufferDescriptor.m_byteCount = prebuildInfo.ResultDataMaxSizeInBytes;

            AZ::RHI::BufferInitRequest blasBufferRequest;
            blasBufferRequest.m_buffer = buffers.m_blasBuffer.get();
            blasBufferRequest.m_descriptor = blasBufferDescriptor;
            resultCode = bufferPools.GetBlasBufferPool()->InitBuffer(blasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS buffer");

            MemoryView& blasMemoryView = static_cast<Buffer*>(buffers.m_blasBuffer.get())->GetMemoryView();
            blasMemoryView.SetName(L"BLAS");
#endif
            return RHI::ResultCode::Success;
        }
    }
}

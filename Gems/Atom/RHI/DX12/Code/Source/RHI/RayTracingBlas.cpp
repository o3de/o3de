/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/RayTracingBlas.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>


namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingBlas> RayTracingBlas::Create()
        {
            return aznew RayTracingBlas;
        }

        uint64_t RayTracingBlas::GetAccelerationStructureByteSize()
        {
            return m_buffers.GetCurrentElement().m_blasBuffer->GetDescriptor().m_byteCount;
        }

        RHI::ResultCode RayTracingBlas::CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::DeviceRayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            Device& device = static_cast<Device&>(deviceBase);
            ID3D12DeviceX* dx12Device = device.GetDevice();

            // advance to the next buffer
            BlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            m_geometryDescs.clear();

            // A BLAS can contain either triangle geometry or procedural geometry; decide based on the descriptor which one to create
            if (descriptor->HasAABB())
            {
                const AZ::Aabb& aabb = descriptor->GetAABB();
                buffers.m_aabbBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor blasBufferDescriptor;
                blasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::CopyRead;
                blasBufferDescriptor.m_byteCount = sizeof(D3D12_RAYTRACING_AABB);
                blasBufferDescriptor.m_alignment = D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT;

                D3D12_RAYTRACING_AABB rtAabb;
                rtAabb.MinX = aabb.GetMin().GetX();
                rtAabb.MinY = aabb.GetMin().GetY();
                rtAabb.MinZ = aabb.GetMin().GetZ();
                rtAabb.MaxX = aabb.GetMax().GetX();
                rtAabb.MaxY = aabb.GetMax().GetY();
                rtAabb.MaxZ = aabb.GetMax().GetZ();

                AZ::RHI::DeviceBufferInitRequest blasBufferRequest;
                blasBufferRequest.m_buffer = buffers.m_aabbBuffer.get();
                blasBufferRequest.m_initialData = &rtAabb;
                blasBufferRequest.m_descriptor = blasBufferDescriptor;
                auto resultCode = bufferPools.GetAabbStagingBufferPool()->InitBuffer(blasBufferRequest);
                if (resultCode != AZ::RHI::ResultCode::Success)
                {
                    AZ_Error("RayTracing", false, "Failed to initialize BLAS buffer index buffer with error code: %d", resultCode);
                }

                D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
                geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
                geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                geometryDesc.AABBs.AABBCount = 1;
                geometryDesc.AABBs.AABBs.StartAddress = static_cast<const DX12::Buffer*>(buffers.m_aabbBuffer.get())->GetMemoryView().GetGpuAddress();
                //geometryDesc.AABBs.AABBs.StrideInBytes = D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT;
                geometryDesc.AABBs.AABBs.StrideInBytes = 0;

                m_geometryDescs.push_back(geometryDesc);
            }
            else
            {
                const RHI::DeviceRayTracingGeometryVector& geometries = descriptor->GetGeometries();

                // build the list of D3D12_RAYTRACING_GEOMETRY_DESC structures
                m_geometryDescs.reserve(geometries.size());
                D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
                for (const RHI::DeviceRayTracingGeometry& geometry : geometries)
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

                    // all BLAS geometry is set to opaque, but can be set to transparent at the TLAS Instance level
                    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                    m_geometryDescs.push_back(geometryDesc);
                }
            }

            // retrieve the required sizes for the scratch and result buffers            
            ZeroMemory(&m_inputs, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));
            m_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            m_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            m_inputs.pGeometryDescs = m_geometryDescs.data();
            m_inputs.NumDescs = aznumeric_cast<UINT>(m_geometryDescs.size());
            m_inputs.Flags = GetAccelerationStructureBuildFlags(descriptor->GetBuildFlags());

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
            dx12Device->GetRaytracingAccelerationStructurePrebuildInfo(&m_inputs, &prebuildInfo);

            prebuildInfo.ScratchDataSizeInBytes = RHI::AlignUp(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
            prebuildInfo.ResultDataMaxSizeInBytes = RHI::AlignUp(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

            // create scratch buffer
            buffers.m_scratchBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor scratchBufferDescriptor;
            scratchBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer;
            scratchBufferDescriptor.m_byteCount = prebuildInfo.ScratchDataSizeInBytes;

            AZ::RHI::DeviceBufferInitRequest scratchBufferRequest;
            scratchBufferRequest.m_buffer = buffers.m_scratchBuffer.get();
            scratchBufferRequest.m_descriptor = scratchBufferDescriptor;
            [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetScratchBufferPool()->InitBuffer(scratchBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS scratch buffer");

            MemoryView& scratchMemoryView = static_cast<Buffer*>(buffers.m_scratchBuffer.get())->GetMemoryView();
            scratchMemoryView.SetName(L"BLAS Scratch");

            // create BLAS buffer
            buffers.m_blasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor blasBufferDescriptor;
            blasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            blasBufferDescriptor.m_byteCount = prebuildInfo.ResultDataMaxSizeInBytes;

            AZ::RHI::DeviceBufferInitRequest blasBufferRequest;
            blasBufferRequest.m_buffer = buffers.m_blasBuffer.get();
            blasBufferRequest.m_descriptor = blasBufferDescriptor;
            resultCode = bufferPools.GetBlasBufferPool()->InitBuffer(blasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS buffer");

            MemoryView& blasMemoryView = static_cast<Buffer*>(buffers.m_blasBuffer.get())->GetMemoryView();
            blasMemoryView.SetName(L"BLAS");
#endif
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode RayTracingBlas::CreateCompactedBuffersInternal(
            [[maybe_unused]] RHI::Device& device,
            RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
            uint64_t compactedBufferSize,
            const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            // advance to the next buffer
            BlasBuffers& buffers = m_buffers.AdvanceCurrentElement();
            // create BLAS buffer
            buffers.m_blasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor blasBufferDescriptor;
            blasBufferDescriptor.m_bindFlags =
                RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            blasBufferDescriptor.m_byteCount = compactedBufferSize;

            AZ::RHI::DeviceBufferInitRequest blasBufferRequest;
            blasBufferRequest.m_buffer = buffers.m_blasBuffer.get();
            blasBufferRequest.m_descriptor = blasBufferDescriptor;
            [[maybe_unused]] auto resultCode = rayTracingBufferPools.GetBlasBufferPool()->InitBuffer(blasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS buffer");

            MemoryView& blasMemoryView = static_cast<Buffer*>(buffers.m_blasBuffer.get())->GetMemoryView();
            blasMemoryView.SetName(L"BLAS");

            const auto* dx12SourceBlas = static_cast<RayTracingBlas*>(sourceBlas.get());
            m_inputs = dx12SourceBlas->m_inputs;
            m_geometryDescs = dx12SourceBlas->m_geometryDescs;
#endif

            return RHI::ResultCode::Success;
        }

#ifdef AZ_DX12_DXR_SUPPORT
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS RayTracingBlas::GetAccelerationStructureBuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags)
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS dxBuildFlags = {};
            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE))
            {
                dxBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::FAST_BUILD))
            {
                dxBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_UPDATE))
            {
                dxBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION))
            {
                dxBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
            }

            return dxBuildFlags;
        }
#endif
    }
}

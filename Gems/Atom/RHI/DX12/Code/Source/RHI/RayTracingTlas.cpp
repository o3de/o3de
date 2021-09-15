/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Math/Matrix3x4.h>
#include <RHI/RayTracingTlas.h>
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
        RHI::Ptr<RayTracingTlas> RayTracingTlas::Create()
        {
            return aznew RayTracingTlas;
        }

        RHI::ResultCode RayTracingTlas::CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingTlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& bufferPools)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            Device& device = static_cast<Device&>(deviceBase);
            ID3D12DeviceX* dx12Device = device.GetDevice();

            // advance to the next buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
            TlasBuffers& buffers = m_buffers[m_currentBufferIndex];

            const RHI::RayTracingTlasInstanceVector& instances = descriptor->GetInstances();
            if (instances.empty())
            {
                // no instances in the scene, clear the TLAS buffers
                buffers.m_tlasBuffer = nullptr;
                buffers.m_tlasInstancesBuffer = nullptr;
                buffers.m_scratchBuffer = nullptr;
                return RHI::ResultCode::Success;
            }
            
            D3D12_GPU_VIRTUAL_ADDRESS tlasInstancesGpuAddress = 0;
            uint32_t numInstances = 0;
            if (descriptor->GetInstancesBuffer() == nullptr)
            {
                numInstances = aznumeric_caster(instances.size());
                uint64_t instanceDescsSizeInBytes = RHI::AlignUp(aznumeric_cast<UINT64>(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances.size()), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
            
                // create instances buffer
                buffers.m_tlasInstancesBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor tlasInstancesBufferDescriptor;
                tlasInstancesBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderRead;
                tlasInstancesBufferDescriptor.m_byteCount = instanceDescsSizeInBytes;
                tlasInstancesBufferDescriptor.m_alignment = D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT;
            
                AZ::RHI::BufferInitRequest tlasInstancesBufferRequest;
                tlasInstancesBufferRequest.m_buffer = buffers.m_tlasInstancesBuffer.get();
                tlasInstancesBufferRequest.m_descriptor = tlasInstancesBufferDescriptor;
                RHI::ResultCode resultCode = bufferPools.GetTlasInstancesBufferPool()->InitBuffer(tlasInstancesBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS instances buffer");
            
                MemoryView& tlasInstancesMemoryView = static_cast<Buffer*>(buffers.m_tlasInstancesBuffer.get())->GetMemoryView();
                tlasInstancesMemoryView.SetName(L"TLAS Instance");
            
                RHI::BufferMapResponse mapResponse;
                resultCode = bufferPools.GetTlasInstancesBufferPool()->MapBuffer(RHI::BufferMapRequest(*buffers.m_tlasInstancesBuffer, 0, instanceDescsSizeInBytes), mapResponse);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to map TLAS instances buffer");
                D3D12_RAYTRACING_INSTANCE_DESC* mappedData = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapResponse.m_data);
            
                ZeroMemory(mappedData, instanceDescsSizeInBytes);
            
                // create each D3D12_RAYTRACING_INSTANCE_DESC structure
                for (uint32_t i = 0; i < instances.size(); ++i)
                {
                    const RHI::RayTracingTlasInstance& instance = instances[i];
                    RayTracingBlas* blas = static_cast<RayTracingBlas*>(instance.m_blas.get());
            
                    mappedData[i].InstanceID = instance.m_instanceID;
                    mappedData[i].InstanceContributionToHitGroupIndex = instance.m_hitGroupIndex;
                    // convert transform to row-major 3x4
                    AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(instance.m_transform);
                    matrix3x4.MultiplyByScale(instance.m_nonUniformScale);
                    matrix3x4.StoreToRowMajorFloat12(&mappedData[i].Transform[0][0]);
                    mappedData[i].AccelerationStructure = static_cast<DX12::Buffer*>(blas->GetBuffers().m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
                    // [GFX TODO][ATOM-5270] Add ray tracing TLAS instance mask support
                    mappedData[i].InstanceMask = 0x1;
                }
            
                bufferPools.GetTlasInstancesBufferPool()->UnmapBuffer(*buffers.m_tlasInstancesBuffer);
                tlasInstancesGpuAddress = tlasInstancesMemoryView.GetGpuAddress();
            }
            else
            {
                AZ_Assert(descriptor->GetNumInstancesInBuffer(), "TLAS InstancesBuffer set but instances count is zero");
                tlasInstancesGpuAddress = static_cast<DX12::Buffer*>(descriptor->GetInstancesBuffer().get())->GetMemoryView().GetGpuAddress();
                buffers.m_tlasInstancesBuffer = descriptor->GetInstancesBuffer();
                numInstances = descriptor->GetNumInstancesInBuffer();
            }
            
            // retrieve the required sizes for the scratch and TLAS buffers
            ZeroMemory(&m_inputs, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));
            m_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            m_inputs.InstanceDescs = tlasInstancesGpuAddress;
            m_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            m_inputs.NumDescs = static_cast<UINT>(numInstances);
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
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS scratch buffer");
            
            MemoryView& scratchMemoryView = static_cast<Buffer*>(buffers.m_scratchBuffer.get())->GetMemoryView();
            scratchMemoryView.SetName(L"TLAS Scratch");
            
            // create TLAS buffer
            buffers.m_tlasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor tlasBufferDescriptor;
            tlasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::RayTracingAccelerationStructure;
            tlasBufferDescriptor.m_byteCount = prebuildInfo.ResultDataMaxSizeInBytes;
            
            AZ::RHI::BufferInitRequest tlasBufferRequest;
            tlasBufferRequest.m_buffer = buffers.m_tlasBuffer.get();
            tlasBufferRequest.m_descriptor = tlasBufferDescriptor;
            resultCode = bufferPools.GetTlasBufferPool()->InitBuffer(tlasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS buffer");
            
            MemoryView& tlasMemoryView = static_cast<Buffer*>(buffers.m_tlasBuffer.get())->GetMemoryView();
            tlasMemoryView.SetName(L"TLAS");
#endif
            return RHI::ResultCode::Success;
        }
    }
}

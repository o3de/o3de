/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/DescriptorContext.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace DX12
    {
        void DescriptorContext::CreateNullDescriptors()
        {
            CreateNullDescriptorsSRV();
            CreateNullDescriptorsUAV();
            CreateNullDescriptorsCBV();
            CreateNullDescriptorsSampler();
        }

        void DescriptorContext::CreateNullDescriptorsSRV()
        {
            const AZStd::array<D3D12_SRV_DIMENSION, 10> validSRVDimensions = { D3D12_SRV_DIMENSION_BUFFER,
                D3D12_SRV_DIMENSION_TEXTURE1D,
                D3D12_SRV_DIMENSION_TEXTURE1DARRAY,
                D3D12_SRV_DIMENSION_TEXTURE2D,
                D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
                D3D12_SRV_DIMENSION_TEXTURE2DMS,
                D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY,
                D3D12_SRV_DIMENSION_TEXTURE3D,
                D3D12_SRV_DIMENSION_TEXTURECUBE,
                D3D12_SRV_DIMENSION_TEXTURECUBEARRAY ,
            };

            for (D3D12_SRV_DIMENSION dimension : validSRVDimensions)
            {
                DescriptorHandle srvDescriptorHandle = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

                D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.Format = DXGI_FORMAT_R32_UINT;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.ViewDimension = dimension;
                m_device->CreateShaderResourceView(nullptr, &desc, GetCpuPlatformHandle(srvDescriptorHandle));
                m_nullDescriptorsSRV[dimension] = srvDescriptorHandle;
            }
        }

        void DescriptorContext::CreateNullDescriptorsUAV()
        {
            const AZStd::array<D3D12_UAV_DIMENSION, 6> UAVDimensions = { D3D12_UAV_DIMENSION_BUFFER,
                D3D12_UAV_DIMENSION_TEXTURE1D,
                D3D12_UAV_DIMENSION_TEXTURE1DARRAY,
                D3D12_UAV_DIMENSION_TEXTURE2D,
                D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
                D3D12_UAV_DIMENSION_TEXTURE3D };

            for (D3D12_UAV_DIMENSION dimension : UAVDimensions)
            {
                DescriptorHandle uavDescriptorHandle = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

                D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.Format = DXGI_FORMAT_R32_UINT;
                desc.ViewDimension = dimension;
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &desc, GetCpuPlatformHandle(uavDescriptorHandle));
                m_nullDescriptorsUAV[dimension] = uavDescriptorHandle;
            }
        }

        void DescriptorContext::CreateNullDescriptorsCBV()
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferDesc = {};
            DescriptorHandle cbvDescriptorHandle = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            m_device->CreateConstantBufferView(&constantBufferDesc, GetCpuPlatformHandle(cbvDescriptorHandle));
            m_nullDescriptorCBV = cbvDescriptorHandle;
        }

        void DescriptorContext::CreateNullDescriptorsSampler()
        {
            m_nullSamplerDescriptor = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            D3D12_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            m_device->CreateSampler(&samplerDesc, GetCpuPlatformHandle(m_nullSamplerDescriptor));
        }

        void DescriptorContext::Init(ID3D12DeviceX* device, RHI::ConstPtr<PlatformLimitsDescriptor> platformLimitsDescriptor)
        {
            m_device = device;

            AZ_Assert(platformLimitsDescriptor.get(), "Platform limits information is missing");
            m_platformLimitsDescriptor = platformLimitsDescriptor;
            m_allowDescriptorHeapCompaction = m_platformLimitsDescriptor->m_allowDescriptorHeapCompaction;
            for (const auto& itr : platformLimitsDescriptor->m_descriptorHeapLimits)
            {
                for (uint32_t shaderVisibleIdx = 0; shaderVisibleIdx < PlatformLimitsDescriptor::NumHeapFlags; ++shaderVisibleIdx)
                {
                    const uint32_t descriptorCountMax = itr.second[shaderVisibleIdx];
                    AZStd::optional<DESCRIPTOR_HEAP_TYPE> heapTypeIdx = DESCRIPTOR_HEAP_TYPENamespace::FromStringToDESCRIPTOR_HEAP_TYPE(itr.first);
                    const D3D12_DESCRIPTOR_HEAP_TYPE type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(heapTypeIdx.value());
                    const D3D12_DESCRIPTOR_HEAP_FLAGS flags = static_cast<D3D12_DESCRIPTOR_HEAP_FLAGS>(shaderVisibleIdx);

                    if (descriptorCountMax)
                    {
                        if (m_allowDescriptorHeapCompaction && IsShaderVisibleCbvSrvUavHeap(type, flags))
                        {
                            //Init the two heaps to help support compaction after fragmentation
                            m_shaderVisibleCbvSrvUavPools[0].Init(
                                m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                descriptorCountMax, platformLimitsDescriptor->m_numShaderVisibleCbvSrvUavStaticHandles);

                            m_shaderVisibleCbvSrvUavPools[1].Init(
                                m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                descriptorCountMax, platformLimitsDescriptor->m_numShaderVisibleCbvSrvUavStaticHandles);
                        }
                        else
                        {
                            GetPool(static_cast<uint32_t>(heapTypeIdx.value()), shaderVisibleIdx).Init(m_device.get(), type, flags, descriptorCountMax, descriptorCountMax);
                        }
                    }
                }
            }

            if (m_allowDescriptorHeapCompaction)
            {
                m_backupStaticHandles.Init(
                    m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                    platformLimitsDescriptor->m_numShaderVisibleCbvSrvUavStaticHandles,
                    platformLimitsDescriptor->m_numShaderVisibleCbvSrvUavStaticHandles);
            }

            CreateNullDescriptors();
        }

        void DescriptorContext::CreateConstantBufferView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            DescriptorHandle& constantBufferView)
        {
            if (constantBufferView.IsNull())
            {
                constantBufferView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = GetCpuPlatformHandle(constantBufferView);

            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
            ConvertBufferView(buffer, bufferViewDescriptor, viewDesc);
            m_device->CreateConstantBufferView(&viewDesc, descriptorHandle);
        }

        void DescriptorContext::CreateShaderResourceView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            DescriptorHandle& shaderResourceView)
        {
            if (shaderResourceView.IsNull())
            {
                shaderResourceView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = GetCpuPlatformHandle(shaderResourceView);

            D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
            ConvertBufferView(buffer, bufferViewDescriptor, viewDesc);

            bool isRayTracingAccelerationStructure = RHI::CheckBitsAll(buffer.GetDescriptor().m_bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure);
            ID3D12Resource* resource = isRayTracingAccelerationStructure ? nullptr : buffer.GetMemoryView().GetMemory();
            m_device->CreateShaderResourceView(resource, &viewDesc, descriptorHandle);
        }

        void DescriptorContext::CreateUnorderedAccessView(
            const Buffer& buffer,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            DescriptorHandle& unorderedAccessView,
            DescriptorHandle& unorderedAccessViewClear)
        {
            if (unorderedAccessView.IsNull())
            {
                unorderedAccessView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE unorderedAccessDescriptor = GetCpuPlatformHandle(unorderedAccessView);

            D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
            ConvertBufferView(buffer, bufferViewDescriptor, viewDesc);
            m_device->CreateUnorderedAccessView(buffer.GetMemoryView().GetMemory(), nullptr, &viewDesc, unorderedAccessDescriptor);

            // Copy the UAV descriptor into the GPU-visible version for clearing.
            if (unorderedAccessViewClear.IsNull())
            {
                unorderedAccessViewClear = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1);

                if (unorderedAccessViewClear.IsNull())
                {
                    AZ_Assert(
                        false,
                        "Descriptor heap ran out of memory for static handles. Please consider increasing the value of NumShaderVisibleCbvSrvUavStaticHandles"
                        "within platformlimits.azasset file for dx12.");
                    return;
                }

                if (m_allowDescriptorHeapCompaction)
                {
                    //We make a copy of static handles in case we need to compact and recreate the shader visible heap
                    m_device->CopyDescriptorsSimple(
                        1, m_backupStaticHandles.GetCpuPlatformHandle(unorderedAccessViewClear), unorderedAccessDescriptor,
                        unorderedAccessViewClear.m_type);
                }
            }
            CopyDescriptor(unorderedAccessViewClear, unorderedAccessView);
        }

        void DescriptorContext::CreateShaderResourceView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            DescriptorHandle& shaderResourceView)
        {
            if (shaderResourceView.IsNull())
            {
                shaderResourceView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = GetCpuPlatformHandle(shaderResourceView);

            D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
            ConvertImageView(image, imageViewDescriptor, viewDesc);
            m_device->CreateShaderResourceView(image.GetMemoryView().GetMemory(), &viewDesc, descriptorHandle);
        }

        void DescriptorContext::CreateUnorderedAccessView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            DescriptorHandle& unorderedAccessView,
            DescriptorHandle& unorderedAccessViewClear)
        {
            if (unorderedAccessView.IsNull())
            {
                unorderedAccessView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE unorderedAccessDescriptor = GetCpuPlatformHandle(unorderedAccessView);

            D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
            ConvertImageView(image, imageViewDescriptor, viewDesc);
            m_device->CreateUnorderedAccessView(image.GetMemoryView().GetMemory(), nullptr, &viewDesc, unorderedAccessDescriptor);

            // Copy the UAV descriptor into the GPU-visible version for clearing.
            if (unorderedAccessViewClear.IsNull())
            {
                unorderedAccessViewClear = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1);

                if (unorderedAccessViewClear.IsNull())
                {
                    AZ_Assert(
                        false,
                        "Descriptor heap ran out of memory for static handles. Please consider increasing the value of "
                        "NumShaderVisibleCbvSrvUavStaticHandles within platformlimits.azasset file for dx12.");
                    return;
                }

                if (m_allowDescriptorHeapCompaction)
                {
                    // We make a copy of static handles in case we need to compact and recreate the shader visible heap
                    m_device->CopyDescriptorsSimple(
                        1, m_backupStaticHandles.GetCpuPlatformHandle(unorderedAccessViewClear), unorderedAccessDescriptor,
                        unorderedAccessViewClear.m_type);
                }
            }
            CopyDescriptor(unorderedAccessViewClear, unorderedAccessView);
        }

        void DescriptorContext::CreateRenderTargetView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            DescriptorHandle& renderTargetView)
        {
            if (renderTargetView.IsNull())
            {
                renderTargetView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptor = GetCpuPlatformHandle(renderTargetView);

            D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
            ConvertImageView(image, imageViewDescriptor, viewDesc);
            m_device->CreateRenderTargetView(image.GetMemoryView().GetMemory(), &viewDesc, renderTargetDescriptor);
        }

        void DescriptorContext::CreateDepthStencilView(
            const Image& image,
            const RHI::ImageViewDescriptor& imageViewDescriptor,
            DescriptorHandle& depthStencilView,
            DescriptorHandle& depthStencilReadView)
        {
            if (depthStencilView.IsNull())
            {
                depthStencilView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor = GetCpuPlatformHandle(depthStencilView);

            if (depthStencilReadView.IsNull())
            {
                depthStencilReadView = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }
            D3D12_CPU_DESCRIPTOR_HANDLE depthStencilReadDescriptor = GetCpuPlatformHandle(depthStencilReadView);

            D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc;
            ConvertImageView(image, imageViewDescriptor, viewDesc);
            m_device->CreateDepthStencilView(image.GetMemoryView().GetMemory(), &viewDesc, depthStencilDescriptor);

            viewDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;

            const bool isStencilFormat = GetStencilFormat(viewDesc.Format) != DXGI_FORMAT_UNKNOWN;
            if (isStencilFormat)
            {
                viewDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
            }

            m_device->CreateDepthStencilView(image.GetMemoryView().GetMemory(), &viewDesc, depthStencilReadDescriptor);
        }

        void DescriptorContext::CreateSampler(const RHI::SamplerState& samplerState, DescriptorHandle& samplerHandle)
        {
            if (samplerHandle.IsNull())
            {
                samplerHandle = AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
            }

            D3D12_SAMPLER_DESC samplerDesc;
            ConvertSamplerState(samplerState, samplerDesc);
            m_device->CreateSampler(&samplerDesc, GetCpuPlatformHandle(samplerHandle));
        }

        void DescriptorContext::ReleaseDescriptor(DescriptorHandle descriptorHandle)
        {
            if (!descriptorHandle.IsNull())
            {
                GetPool(descriptorHandle.m_type, descriptorHandle.m_flags).ReleaseHandle(descriptorHandle);
            }
        }

        DescriptorTable DescriptorContext::CreateDescriptorTable(
            D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t descriptorCount, ShaderResourceGroup* srg)
        {
            if (m_allowDescriptorHeapCompaction  && !m_compactionInProgress)
            {
                // Track active SRGs in case we need to compact the shader visible cbv_srv_uav heap
                AZStd::scoped_lock lock{ m_srgMapMutex };
                auto iter = m_srgAllocations.find(srg);
                if (iter == m_srgAllocations.end())
                {
                    m_srgAllocations.emplace(srg, 1);
                }
                else
                {
                    m_srgAllocations[srg]++;
                }
            }

            return GetPool(descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).AllocateTable(descriptorCount);
        }

        void DescriptorContext::ReleaseDescriptorTable(DescriptorTable table, ShaderResourceGroup* srg)
        {
            if (m_allowDescriptorHeapCompaction && !m_compactionInProgress)
            {
                //Track active SRGs in case we need to compact the shader visible cbv_srv_uav heap
                AZStd::scoped_lock lock{ m_srgMapMutex };
                auto iter = m_srgAllocations.find(srg);
                AZ_Assert(iter != m_srgAllocations.end(), "Srg entry not found");
                m_srgAllocations[srg]--;
                if (m_srgAllocations[srg] == 0)
                {
                    m_srgAllocations.erase(srg);
                }
            }

            GetPool(table.GetType(), table.GetFlags()).ReleaseTable(table);
        }
        
        void DescriptorContext::UpdateDescriptorTableRange(
            DescriptorTable gpuDestinationTable,
            const DescriptorHandle* cpuSourceDescriptors,
            D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        {
            const uint32_t DescriptorCount = gpuDestinationTable.GetSize();

            // Resolve source descriptors to platform handles.
            AZStd::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuSourceHandles;
            cpuSourceHandles.reserve(DescriptorCount);
            for (uint32_t i = 0; i < DescriptorCount; ++i)
            {
                cpuSourceHandles.push_back(GetCpuPlatformHandle(cpuSourceDescriptors[i]));
            }

            // Resolve destination descriptor to platform handle.
            D3D12_CPU_DESCRIPTOR_HANDLE gpuDestinationHandle = GetCpuPlatformHandleForTable(gpuDestinationTable);

            // An array of descriptor sizes for each range. We just want N ranges with 1 descriptor each.
            AZStd::vector<uint32_t> rangeCounts(DescriptorCount, 1);

            //We are gathering N source descriptors into a contiguous destination table.
            m_device->CopyDescriptors(
                1,                      // Number of destination ranges.
                &gpuDestinationHandle,  // Destination range array.
                &DescriptorCount,       // Number of destination table elements in each range.
                DescriptorCount,        // Number of source ranges.
                cpuSourceHandles.data(),// Source range array
                rangeCounts.data(),     // Number of elements in each source range.
                heapType);
        }

        void DescriptorContext::CopyDescriptor(DescriptorHandle dest, DescriptorHandle source)
        {
            AZ_Assert(dest.m_type == source.m_type, "Cannot copy descriptors from different heaps");
            AZ_Assert(!source.IsShaderVisible(), "The source descriptor cannot be shader visible.");
            m_device->CopyDescriptorsSimple(1, GetCpuPlatformHandle(dest), GetCpuPlatformHandle(source), dest.m_type);
        }

        void DescriptorContext::GarbageCollect()
        {
            AZ_PROFILE_SCOPE(RHI, "DescriptorContext: GarbageCollect(DX12)");
            for (const auto& itr : m_platformLimitsDescriptor->m_descriptorHeapLimits)
            {
                for (uint32_t shaderVisibleIdx = 0; shaderVisibleIdx < PlatformLimitsDescriptor::NumHeapFlags; ++shaderVisibleIdx)
                {
                    const uint32_t descriptorCountMax = itr.second[shaderVisibleIdx];
                    AZStd::optional<DESCRIPTOR_HEAP_TYPE> heapTypeIdx = DESCRIPTOR_HEAP_TYPENamespace::FromStringToDESCRIPTOR_HEAP_TYPE(itr.first);
                    if (descriptorCountMax)
                    {
                        GetPool(static_cast<uint32_t>(heapTypeIdx.value()), shaderVisibleIdx).GarbageCollect();
                    }
                }
            }

            if (m_allowDescriptorHeapCompaction)
            {
                m_backupStaticHandles.GarbageCollect();
            }
        }

        DescriptorTable DescriptorContext::AllocateTable(
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            uint32_t count)
        {
            return GetPool(type, flags).AllocateTable(count);
        }

        DescriptorHandle DescriptorContext::AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t count)
        {
            return GetPool(type, flags).AllocateHandle(count);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorContext::GetCpuPlatformHandle(DescriptorHandle handle) const
        {
            return GetPool(handle.m_type, handle.m_flags).GetCpuPlatformHandle(handle);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorContext::GetGpuPlatformHandle(DescriptorHandle handle) const
        {
            return GetPool(handle.m_type, handle.m_flags).GetGpuPlatformHandle(handle);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorContext::GetCpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            return GetPool(descTable.GetOffset().m_type, descTable.GetOffset().m_flags).GetCpuPlatformHandleForTable(descTable);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorContext::GetGpuPlatformHandleForTable(DescriptorTable descTable) const
        {
            return GetPool(descTable.GetOffset().m_type, descTable.GetOffset().m_flags).GetGpuPlatformHandleForTable(descTable);
        }

        DescriptorHandle DescriptorContext::GetNullHandleSRV(D3D12_SRV_DIMENSION dimension) const
        {
            auto iter = m_nullDescriptorsSRV.find(dimension);
            if (iter != m_nullDescriptorsSRV.end())
            {
                return iter->second;
            }
            else
            {
                return DescriptorHandle();
            }
        }

        DescriptorHandle DescriptorContext::GetNullHandleUAV(D3D12_UAV_DIMENSION dimension) const
        {
            auto iter = m_nullDescriptorsUAV.find(dimension);
            if (iter != m_nullDescriptorsUAV.end())
            {
                return iter->second;
            }
            else
            {
                return DescriptorHandle();
            }
        }

        DescriptorHandle DescriptorContext::GetNullHandleCBV() const
        {
            return m_nullDescriptorCBV;
        }

        DescriptorHandle DescriptorContext::GetNullHandleSampler() const
        {
            return m_nullSamplerDescriptor;
        }

        void DescriptorContext::SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList) const
        {
            ID3D12DescriptorHeap* heaps[2];
            heaps[0] = GetPool(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).GetPlatformHeap();
            heaps[1] = GetPool(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).GetPlatformHeap();
            commandList->SetDescriptorHeaps(2, heaps);
        }

        ID3D12DeviceX* DescriptorContext::GetDevice()
        {
            return m_device.get();
        }

        DescriptorPool& DescriptorContext::GetPool(uint32_t type, uint32_t flag)
        {
            AZ_Assert(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Trying to get pool with invalid type: [%d]", type);
            AZ_Assert(flag < NumHeapFlags, "Trying to get pool with invalid flag: [%d]", flag);

            if (m_allowDescriptorHeapCompaction && IsShaderVisibleCbvSrvUavHeap(type, flag))
            {
                return m_shaderVisibleCbvSrvUavPools[m_currentHeapIndex];
            }
            else
            {
                return m_pools[type][flag];
            }
        }

        const DescriptorPool& DescriptorContext::GetPool(uint32_t type, uint32_t flag) const
        {
            AZ_Assert(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Trying to get pool with invalid type: [%d]", type);
            AZ_Assert(flag < NumHeapFlags, "Trying to get pool with invalid flag: [%d]", flag);
            if (m_allowDescriptorHeapCompaction && IsShaderVisibleCbvSrvUavHeap(type, flag))
            {
                return m_shaderVisibleCbvSrvUavPools[m_currentHeapIndex];
            }
            else
            {
                return m_pools[type][flag];
            }
        }

        RHI::ResultCode DescriptorContext::CompactDescriptorHeap()
        {
            //Check if heap compaction is enabled by the user. Since there is an overhead associated with heap compaction it is not enabled by default
            if(!m_allowDescriptorHeapCompaction)
            {
                AZ_Assert(
                    false,
                    "Descriptor heap Compaction not allowed. Please consider increasing number of handles allowed for the second value"
                    "of DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV or enabling AllowDescriptorHeapCompaction within platformlimits.azasset file for dx12.");
                return RHI::ResultCode::OutOfMemory;
            }

            //We need to ping-pong between two heaps as we cannot compact the active heap without updating it and that is not allowed as
            //we need to keep that gpu memory untouched until GPU is finished consuming which can take up to 3 frames. 
            m_compactionInProgress = true;
            DescriptorPool& srcPool = GetPool(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

            //Update the currently active heap index
            m_currentHeapIndex = !m_currentHeapIndex; 
            DescriptorPool& destPool = GetPool(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

            //Copy over all the static handles first
            for (size_t i = 0; i < m_platformLimitsDescriptor->m_numShaderVisibleCbvSrvUavStaticHandles; i++)
            {
                DescriptorHandle srcHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, static_cast<uint32_t>(i));
                DescriptorHandle destHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, static_cast<uint32_t>(i));
                m_device->CopyDescriptorsSimple(1, destPool.GetCpuPlatformHandle(destHandle), m_backupStaticHandles.GetCpuPlatformHandle(srcHandle), destHandle.m_type);
            }

            //Clone the allocator of the source pool into the destination pool
            srcPool.CloneAllocator(destPool.GetAllocator());

            {
                //The mutex is here 'just in case' Compaction is called from more than one thread.
                AZStd::scoped_lock lock{ m_srgMapMutex };
                //Re-update all the descriptor tables associated with active SRGs
                for (const auto& [srg, numAllocations] : m_srgAllocations)
                {
                    RHI::ResultCode resultCode = static_cast<ShaderResourceGroupPool*>(srg->GetPool())->UpdateDescriptorTableAfterCompaction(*srg, srg->GetData());
                    if (resultCode != RHI::ResultCode::Success)
                    {
                        return resultCode;
                    }
                }
            }

            //Clear the allocator of the source pool
            srcPool.ClearAllocator();

            m_compactionInProgress = false;

            return RHI::ResultCode::Success;
        }

        bool DescriptorContext::IsShaderVisibleCbvSrvUavHeap(uint32_t type, uint32_t flag) const
        {
            return type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && flag == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }
    }
}

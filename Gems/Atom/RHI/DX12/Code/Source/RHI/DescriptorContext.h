/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RHI/DescriptorPool.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceImage.h>
#include <AzCore/std/containers/unordered_map.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;
        class Image;
        class PlatformLimitsDescriptor;

        //! Manages pools of descriptors.
        class DescriptorContext
        {
        public:
            AZ_CLASS_ALLOCATOR(DescriptorContext, AZ::SystemAllocator);

            DescriptorContext() = default;

            void Init(ID3D12DeviceX* device, RHI::ConstPtr<PlatformLimitsDescriptor> platformLimitsDescriptor);

            void CreateConstantBufferView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& constantBufferView,
                DescriptorHandle& staticView);

            void CreateShaderResourceView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& shaderResourceView,
                DescriptorHandle& staticView);

            void CreateUnorderedAccessView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& unorderedAccessView,
                DescriptorHandle& unorderedAccessViewClear,
                DescriptorHandle& staticView);

            void CreateShaderResourceView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& shaderResourceView,
                DescriptorHandle& staticView);

            void CreateUnorderedAccessView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& unorderedAccessView,
                DescriptorHandle& unorderedAccessViewClear,
                DescriptorHandle& staticView);

            void CreateRenderTargetView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& renderTargetView);

            void CreateDepthStencilView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& depthStencilView,
                DescriptorHandle& depthStencilReadView);

            void CreateSampler(
                const RHI::SamplerState& samplerState,
                DescriptorHandle& samplerHandle);

            void ReleaseDescriptor(DescriptorHandle descriptorHandle);

            void ReleaseStaticDescriptor(DescriptorHandle index);

            //! Creates a GPU-visible descriptor table.
            //! @param descriptorHeapType The descriptor heap to allocate from.
            //! @param descriptorCount The number of descriptors to allocate.
            DescriptorTable CreateDescriptorTable(
                D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t descriptorCount);

            //! Retrieve a descriptor handle to the start of the static region of the shader-visible CBV_SRV_UAV heap
            D3D12_GPU_DESCRIPTOR_HANDLE GetBindlessGpuPlatformHandle() const;

            //! Releases a GPU-visible descriptor table.
            //! @param descriptorHeapType The descriptor heap to allocate from.
            void ReleaseDescriptorTable(DescriptorTable descriptorTable);
            
            //! Performs a gather of disjoint CPU-side descriptors and copies to a contiguous GPU-side descriptor table.
            //! @param gpuDestinationTable The destination descriptor table that the descriptors will be uploaded to.
            //!     This must be the table specifically for a given range of descriptors, so if
            //!     the user created a table with multiple ranges, they are required to partition
            //!     that table and call this method multiple times with each range partition.
            //!
            //! @param cpuSourceDescriptors The CPU descriptors being gathered and copied to the destination table. 
            //!     The number of elements must match the size of the destination table.
            //! @param heapType The type of heap being updated.
            void UpdateDescriptorTableRange(
                DescriptorTable gpuDestinationTable,
                const DescriptorHandle* cpuSourceDescriptors,
                D3D12_DESCRIPTOR_HEAP_TYPE heapType);
           
            DescriptorHandle GetNullHandleSRV(D3D12_SRV_DIMENSION dimension) const;
            DescriptorHandle GetNullHandleUAV(D3D12_UAV_DIMENSION dimension) const;
            DescriptorHandle GetNullHandleCBV() const;
            DescriptorHandle GetNullHandleSampler() const;

            D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPlatformHandle(DescriptorHandle handle) const;
            D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPlatformHandle(DescriptorHandle handle) const;
            D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPlatformHandleForTable(DescriptorTable descTable) const;
            D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPlatformHandleForTable(DescriptorTable descTable) const;

            void SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList) const;

            void GarbageCollect();

            ID3D12DeviceX* GetDevice();

        private:
            void CopyDescriptor(DescriptorHandle dst, DescriptorHandle src);

            // Accepts a descriptor allocated from the CPU visible heap and creates a copy in the shader-
            // visible heap in the static region
            DescriptorHandle AllocateStaticDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle);

            void CreateNullDescriptors();
            void CreateNullDescriptorsSRV();
            void CreateNullDescriptorsUAV();
            void CreateNullDescriptorsCBV();
            void CreateNullDescriptorsSampler();

            DescriptorPool& GetPool(uint32_t type, uint32_t flag);
            const DescriptorPool& GetPool(uint32_t type, uint32_t flag) const;

            //! Allocates a Descriptor table which describes a contiguous range of descriptor handles
            DescriptorTable AllocateTable(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t count);

            //! Allocates a single descriptor handle
            DescriptorHandle AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t count);

            bool IsShaderVisibleCbvSrvUavHeap(uint32_t type, uint32_t flag) const;

            static const uint32_t NumHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE + 1;
            static const uint32_t s_descriptorCountMax[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES][NumHeapFlags];

            RHI::Ptr<ID3D12DeviceX> m_device;

            DescriptorPool m_pools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES][NumHeapFlags];

            // The static pool is a region of the shader-visible descriptor heap used to store descriptors that persist for the
            // lifetime of the resource view they reference
            DescriptorPool m_staticPool;
            // This table binds the entire range of CBV_SRV_UAV descriptor handles in the shader visible heap
            DescriptorTable m_staticTable;

            AZStd::unordered_map<D3D12_SRV_DIMENSION, DescriptorHandle> m_nullDescriptorsSRV;
            AZStd::unordered_map<D3D12_UAV_DIMENSION, DescriptorHandle> m_nullDescriptorsUAV;
            DescriptorHandle m_nullDescriptorCBV;
            DescriptorHandle m_nullSamplerDescriptor;

            RHI::ConstPtr<PlatformLimitsDescriptor> m_platformLimitsDescriptor;

            // Offset from the shader-visible descriptor heap start to the first static descriptor handle
            uint32_t m_staticDescriptorOffset = 0;
        };
    }
}

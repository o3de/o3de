/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Image.h>

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
            AZ_CLASS_ALLOCATOR(DescriptorContext, AZ::SystemAllocator, 0);

            DescriptorContext() = default;

            void Init(ID3D12DeviceX* device, RHI::ConstPtr<PlatformLimitsDescriptor> platformLimitsDescriptor);

            void CreateConstantBufferView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& constantBufferView);

            void CreateShaderResourceView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& shaderResourceView);

            void CreateUnorderedAccessView(
                const Buffer& buffer,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                DescriptorHandle& unorderedAccessView,
                DescriptorHandle& unorderedAccessViewClear);

            void CreateShaderResourceView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& shaderResourceView);

            void CreateUnorderedAccessView(
                const Image& image,
                const RHI::ImageViewDescriptor& imageViewDescriptor,
                DescriptorHandle& unorderedAccessView,
                DescriptorHandle& unorderedAccessViewClear);

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


            //! Creates a GPU-visible descriptor table.
            //! @param descriptorHeapType The descriptor heap to allocate from.
            //! @param descriptorCount The number of descriptors to allocate.
            DescriptorTable CreateDescriptorTable(
                D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                uint32_t descriptorCount);

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

            void SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList) const;

            void GarbageCollect();

            ID3D12DeviceX* GetDevice();

        private:
            void CopyDescriptor(DescriptorHandle dst, DescriptorHandle src);

            void CreateNullDescriptors();
            void CreateNullDescriptorsSRV();
            void CreateNullDescriptorsUAV();
            void CreateNullDescriptorsCBV();
            void CreateNullDescriptorsSampler();

            DescriptorPool& GetPool(uint32_t type, uint32_t flag);
            const DescriptorPool& GetPool(uint32_t type, uint32_t flag) const;

            DescriptorTable Allocate(
                D3D12_DESCRIPTOR_HEAP_TYPE type,
                D3D12_DESCRIPTOR_HEAP_FLAGS flags,
                uint32_t count);

            static const uint32_t NumHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE + 1;
            static const uint32_t s_descriptorCountMax[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES][NumHeapFlags];

            RHI::Ptr<ID3D12DeviceX> m_device;

            DescriptorPool m_pools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES][NumHeapFlags];

            AZStd::unordered_map<D3D12_SRV_DIMENSION, DescriptorHandle> m_nullDescriptorsSRV;
            AZStd::unordered_map<D3D12_UAV_DIMENSION, DescriptorHandle> m_nullDescriptorsUAV;
            DescriptorHandle m_nullDescriptorCBV;
            DescriptorHandle m_nullSamplerDescriptor;

            RHI::ConstPtr<PlatformLimitsDescriptor> m_platformLimitsDescriptor;
        };
    }
}

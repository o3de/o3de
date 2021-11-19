/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <AzCore/Debug/EventTrace.h>


namespace AZ
{
    namespace DX12
    {
        template<typename T, typename U>
        AZStd::vector<DescriptorHandle> ShaderResourceGroupPool::GetSRVsFromImageViews(const AZStd::array_view<RHI::ConstPtr<T>>& imageViews, D3D12_SRV_DIMENSION dimension)
        {
            AZStd::vector<DescriptorHandle> cpuSourceDescriptors(imageViews.size(), m_descriptorContext->GetNullHandleSRV(dimension));

            for (size_t i = 0; i < cpuSourceDescriptors.size(); ++i)
            {
                if (imageViews[i])
                {
                    cpuSourceDescriptors[i] = AZStd::static_pointer_cast<const U>(imageViews[i])->GetReadDescriptor();
                }
            }

            return cpuSourceDescriptors;
        }

        template<typename T, typename U>
        AZStd::vector<DescriptorHandle> ShaderResourceGroupPool::GetUAVsFromImageViews(const AZStd::array_view<RHI::ConstPtr<T>>& imageViews, D3D12_UAV_DIMENSION dimension)
        {
            AZStd::vector<DescriptorHandle> cpuSourceDescriptors(imageViews.size(), m_descriptorContext->GetNullHandleUAV(dimension));
            for (size_t i = 0; i < cpuSourceDescriptors.size(); ++i)
            {
                if (imageViews[i])
                {
                    cpuSourceDescriptors[i] = AZStd::static_pointer_cast<const U>(imageViews[i])->GetReadWriteDescriptor();
                }
            }

            return cpuSourceDescriptors;
        }

        AZStd::vector<DescriptorHandle> ShaderResourceGroupPool::GetCBVsFromBufferViews(const AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>& bufferViews)
        {
            AZStd::vector<DescriptorHandle> cpuSourceDescriptors(bufferViews.size(), m_descriptorContext->GetNullHandleCBV());

            for (size_t i = 0; i < bufferViews.size(); ++i)
            {
                if (bufferViews[i])
                {
                    cpuSourceDescriptors[i] = AZStd::static_pointer_cast<const BufferView>(bufferViews[i])->GetConstantDescriptor();
                }
            }
            return cpuSourceDescriptors;
        }

        RHI::Ptr<ShaderResourceGroupPool> ShaderResourceGroupPool::Create()
        {
            return aznew ShaderResourceGroupPool();
        }

        RHI::ResultCode ShaderResourceGroupPool::InitInternal(
            RHI::Device& deviceBase,
            const RHI::ShaderResourceGroupPoolDescriptor& descriptor)
        {
            Device& device = static_cast<Device&>(deviceBase);
            m_descriptorContext = &device.GetDescriptorContext();

            const RHI::ShaderResourceGroupLayout& layout = *descriptor.m_layout;
            m_constantBufferSize = RHI::AlignUp(layout.GetConstantDataSize(), Alignment::Constant);
            m_constantBufferRingSize = m_constantBufferSize * RHI::Limits::Device::FrameCountMax;
            m_viewsDescriptorTableSize = layout.GetGroupSizeForBuffers() + layout.GetGroupSizeForImages();
            m_viewsDescriptorTableRingSize = m_viewsDescriptorTableSize * RHI::Limits::Device::FrameCountMax;
            m_samplersDescriptorTableSize = layout.GetGroupSizeForSamplers();
            m_samplersDescriptorTableRingSize = m_samplersDescriptorTableSize * RHI::Limits::Device::FrameCountMax;

            // Buffers occupy the first region of the table, then images.
            m_descriptorTableBufferOffset = 0;
            m_descriptorTableImageOffset = layout.GetGroupSizeForBuffers();

            // Unbounded arrays each have their own descriptor tables
            m_unboundedArrayCount = layout.GetGroupSizeForBufferUnboundedArrays() + layout.GetGroupSizeForImageUnboundedArrays();

            if (m_constantBufferSize)
            {
                MemoryPoolSubAllocator::Descriptor allocatorDescriptor;
                allocatorDescriptor.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
                allocatorDescriptor.m_elementSize = m_constantBufferRingSize;
                m_constantAllocator.Init(allocatorDescriptor, device.GetConstantMemoryPageAllocator());
            }

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::ShutdownInternal()
        {
            if (m_constantBufferSize)
            {
                m_constantAllocator.Shutdown();
            }
            Base::ShutdownInternal();
        }

        RHI::ResultCode ShaderResourceGroupPool::InitGroupInternal(RHI::ShaderResourceGroup& groupBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);

            const uint32_t copyCount = RHI::Limits::Device::FrameCountMax;

            if (m_constantBufferSize)
            {
                group.m_constantMemoryView = MemoryView(m_constantAllocator.Allocate(m_constantBufferRingSize, RHI::Alignment::Constant), MemoryViewType::Buffer);
                CpuVirtualAddress cpuAddress = group.m_constantMemoryView.Map(RHI::HostMemoryAccess::Write);
                GpuVirtualAddress gpuAddress = group.m_constantMemoryView.GetGpuAddress();

                for (uint32_t i = 0; i < copyCount; ++i)
                {
                    ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[i];
                    compiledData.m_gpuConstantAddress = gpuAddress + m_constantBufferSize * i;
                    compiledData.m_cpuConstantAddress = cpuAddress + m_constantBufferSize * i;
                }
            }
   
            if (m_samplersDescriptorTableSize)
            {
                group.m_samplersDescriptorTable = m_descriptorContext->CreateDescriptorTable(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_samplersDescriptorTableRingSize, &group);

                if (!group.m_samplersDescriptorTable.IsValid())
                {
                    AZ_Error(
                        "ShaderResourceGroupPool", false,
                        "Descriptor context failed to allocate sampler descriptor table. Please consider increasing number of handles "
                        "allowed for the second value of DESCRIPTOR_HEAP_TYPE_SAMPLER within platformlimits.azasset file for dx12.");
                    return RHI::ResultCode::OutOfMemory;
                }

                for (uint32_t i = 0; i < copyCount; ++i)
                {
                    const DescriptorHandle descriptorHandle = group.m_samplersDescriptorTable.GetOffset() + m_samplersDescriptorTableSize * i;

                    ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[i];
                    compiledData.m_gpuSamplersDescriptorHandle = m_descriptorContext->GetGpuPlatformHandleForTable(DescriptorTable(descriptorHandle, static_cast<uint16_t>(m_samplersDescriptorTableSize)));
                }
            }

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(resourceBase);

            if (m_constantBufferSize)
            {
                group.m_constantMemoryView.Unmap(RHI::HostMemoryAccess::Write);
                m_constantAllocator.DeAllocate(group.m_constantMemoryView.m_memoryAllocation);
            }

            if (m_viewsDescriptorTableSize)
            {
                if (group.m_viewsDescriptorTable.IsValid())
                {
                    m_descriptorContext->ReleaseDescriptorTable(group.m_viewsDescriptorTable, &group);
                }
            }

            if (m_samplersDescriptorTableSize)
            {
                if (group.m_viewsDescriptorTable.IsValid())
                {
                    m_descriptorContext->ReleaseDescriptorTable(group.m_samplersDescriptorTable, &group);
                }
            }

            for (uint32_t unboundedArrayindex = 0; unboundedArrayindex < (ShaderResourceGroupCompiledData::MaxUnboundedArrays * RHI::Limits::Device::FrameCountMax); ++unboundedArrayindex)
            {
                if (group.m_unboundedDescriptorTables[unboundedArrayindex].IsValid())
                {
                    m_descriptorContext->ReleaseDescriptorTable(group.m_unboundedDescriptorTables[unboundedArrayindex], &group);
                }
            }

            group.m_compiledDataIndex = 0;
            group.m_compiledData.fill(ShaderResourceGroupCompiledData());

            Base::ShutdownResourceInternal(resourceBase);
        }

        RHI::ResultCode ShaderResourceGroupPool::CompileGroupInternal(
            RHI::ShaderResourceGroup& groupBase,
            const RHI::ShaderResourceGroupData& groupData)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);
            auto& device = static_cast<Device&>(GetDevice());
            group.m_compiledDataIndex = (group.m_compiledDataIndex + 1) % RHI::Limits::Device::FrameCountMax;

            if (!groupData.IsAnyResourceTypeUpdated())
            {
                return RHI::ResultCode::Success;
            }

            if (m_constantBufferSize &&
                groupData.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::ConstantDataMask)))
            {
                memcpy(group.GetCompiledData().m_cpuConstantAddress, groupData.GetConstantData().data(), groupData.GetConstantData().size());
            }

            if (m_viewsDescriptorTableSize &&
                groupData.IsResourceTypeEnabledForCompilation(
                    static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::ImageViewMask) |
                    static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::BufferViewMask)))
            {
                //Lazy initialization for cbv/srv/uav Descriptor Tables
                if (!group.m_viewsDescriptorTable.IsValid())
                {
                    group.m_viewsDescriptorTable = m_descriptorContext->CreateDescriptorTable(
                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_viewsDescriptorTableRingSize, &group);

                    if (!group.m_viewsDescriptorTable.IsValid())
                    {
                        //We have support for compacting D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV (if applicable) so try that.
                        device.DescriptorHeapCompactionNeeded();
                        return RHI::ResultCode::Success;
                    }

                    CacheGpuHandlesForViews(group);
                }

                const DescriptorTable descriptorTable(
                    group.m_viewsDescriptorTable.GetOffset() + group.m_compiledDataIndex * m_viewsDescriptorTableSize,
                    static_cast<uint16_t>(m_viewsDescriptorTableSize));

                UpdateViewsDescriptorTable(descriptorTable, groupData);
            }

            if (m_unboundedArrayCount &&
                groupData.IsResourceTypeEnabledForCompilation(
                    static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::ImageViewUnboundedArrayMask) |
                    static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::BufferViewUnboundedArrayMask)))
            {
                UpdateUnboundedArrayDescriptorTables(group, groupData);
            }

            if (m_samplersDescriptorTableSize &&
                groupData.IsResourceTypeEnabledForCompilation(
                    static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::SamplerMask)))
            {
                const DescriptorTable descriptorTable(
                    group.m_samplersDescriptorTable.GetOffset() + group.m_compiledDataIndex * m_samplersDescriptorTableSize,
                    static_cast<uint16_t>(m_samplersDescriptorTableSize));

                UpdateSamplersDescriptorTable(descriptorTable, groupData);
            }

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::CacheGpuHandlesForViews(ShaderResourceGroup& group)
        {
            for (uint32_t i = 0; i < RHI::Limits::Device::FrameCountMax; ++i)
            {
                const DescriptorHandle descriptorHandle = group.m_viewsDescriptorTable.GetOffset() + m_viewsDescriptorTableSize * i;

                ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[i];
                compiledData.m_gpuViewsDescriptorHandle = m_descriptorContext->GetGpuPlatformHandleForTable(
                    DescriptorTable(descriptorHandle, static_cast<uint16_t>(m_viewsDescriptorTableSize)));
            }
        }

        void ShaderResourceGroupPool::UpdateViewsDescriptorTable(DescriptorTable descriptorTable, const RHI::ShaderResourceGroupData& groupData)
        {
            const RHI::ShaderResourceGroupLayout& groupLayout = *groupData.GetLayout();

            uint32_t shaderInputIndex = 0;

            for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : groupLayout.GetShaderInputListForBuffers())
            {
                const RHI::ShaderInputBufferIndex bufferInputIndex(shaderInputIndex);

                AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> bufferViews = groupData.GetBufferViewArray(bufferInputIndex);
                D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType = ConvertShaderInputBufferAccess(shaderInputBuffer.m_access);
                AZStd::vector<DescriptorHandle> descriptorHandles;
                switch (descriptorRangeType)
                {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    {
                        descriptorHandles = GetSRVsFromImageViews< RHI::BufferView, BufferView> (bufferViews, D3D12_SRV_DIMENSION_BUFFER);
                        break;
                    }
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    {
                        descriptorHandles = GetUAVsFromImageViews<RHI::BufferView, BufferView>(bufferViews, D3D12_UAV_DIMENSION_BUFFER);
                        break;
                    }
                    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    {
                        descriptorHandles = GetCBVsFromBufferViews(bufferViews);
                        break;
                    }
                    default:
                        AZ_Assert(false, "Unhandled D3D12_DESCRIPTOR_RANGE_TYPE enumeration");
                        break;
                }

                UpdateDescriptorTableRange(descriptorTable, descriptorHandles, bufferInputIndex);
                ++shaderInputIndex;
            }

            shaderInputIndex = 0;

            for (const RHI::ShaderInputImageDescriptor& shaderInputImage : groupLayout.GetShaderInputListForImages())
            {
                const RHI::ShaderInputImageIndex imageInputIndex(shaderInputIndex);

                AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> imageViews = groupData.GetImageViewArray(imageInputIndex);
                D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType = ConvertShaderInputImageAccess(shaderInputImage.m_access);

                AZStd::vector<DescriptorHandle> descriptorHandles;
                switch (descriptorRangeType)
                {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    {
                        descriptorHandles =
                            GetSRVsFromImageViews<RHI::ImageView, ImageView>(imageViews, ConvertSRVDimension(shaderInputImage.m_type));
                        break;
                    }
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    {
                        descriptorHandles =
                            GetUAVsFromImageViews<RHI::ImageView, ImageView>(imageViews, ConvertUAVDimension(shaderInputImage.m_type));
                        break;
                    }
                    default:
                    AZ_Assert(false, "Unhandled D3D12_DESCRIPTOR_RANGE_TYPE enumeration");
                    break;
                }

                UpdateDescriptorTableRange(descriptorTable, descriptorHandles, imageInputIndex);
                ++shaderInputIndex;
            }
        }

        void ShaderResourceGroupPool::UpdateSamplersDescriptorTable(DescriptorTable descriptorTable, const RHI::ShaderResourceGroupData& groupData)
        {
            const RHI::ShaderResourceGroupLayout& groupLayout = *groupData.GetLayout();
            const size_t shaderInputSize = groupLayout.GetShaderInputListForSamplers().size();
            for (size_t shaderInputIndex = 0; shaderInputIndex < shaderInputSize; ++shaderInputIndex)
            {
                const RHI::ShaderInputSamplerIndex samplerInputIndex(shaderInputIndex);

                AZStd::array_view<RHI::SamplerState> samplers = groupData.GetSamplerArray(samplerInputIndex);
                UpdateDescriptorTableRange(descriptorTable, samplerInputIndex, samplers);
            }
        }

        void ShaderResourceGroupPool::UpdateUnboundedArrayDescriptorTables(ShaderResourceGroup& group, const RHI::ShaderResourceGroupData& groupData)
        {
            const RHI::ShaderResourceGroupLayout& groupLayout = *groupData.GetLayout();
            auto& device = static_cast<Device&>(GetDevice());
            uint32_t shaderInputIndex = 0;

            // process buffer unbounded arrays
            for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray : groupLayout.GetShaderInputListForBufferUnboundedArrays())
            {
                const RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayInputIndex(shaderInputIndex);
                AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> bufferViews = groupData.GetBufferViewUnboundedArray(bufferUnboundedArrayInputIndex);

                uint32_t tableIndex = shaderInputIndex * RHI::Limits::Device::FrameCountMax + group.m_compiledDataIndex;

                // resize the descriptor table allocation if necessary
                if (group.m_unboundedDescriptorTables[tableIndex].GetSize() != bufferViews.size())
                {
                    if (group.m_unboundedDescriptorTables[tableIndex].IsValid())
                    {
                        m_descriptorContext->ReleaseDescriptorTable(group.m_unboundedDescriptorTables[tableIndex], &group);
                        group.m_unboundedDescriptorTables[tableIndex] = DescriptorTable{};
                    }

                    if (!bufferViews.empty())
                    {
                        group.m_unboundedDescriptorTables[tableIndex] = m_descriptorContext->CreateDescriptorTable(
                            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(bufferViews.size()), &group);
                        
                        if (!group.m_unboundedDescriptorTables[tableIndex].IsValid())
                        {
                            // We have support for compacting D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV (if applicable) so try that.
                            device.DescriptorHeapCompactionNeeded();
                            return;
                        }

                        ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[group.m_compiledDataIndex];
                        compiledData.m_gpuUnboundedArraysDescriptorHandles[shaderInputIndex] = m_descriptorContext->GetGpuPlatformHandleForTable(group.m_unboundedDescriptorTables[tableIndex]);
                    }
                }
                
                const DescriptorTable descriptorTable(group.m_unboundedDescriptorTables[tableIndex].GetOffset(), static_cast<uint16_t>(bufferViews.size()));
                UpdateUnboundedBuffersDescTable(descriptorTable, groupData, shaderInputIndex, shaderInputBufferUnboundedArray.m_access);
                ++shaderInputIndex;
            }

            // process image unbounded arrays
            for (const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray : groupLayout.GetShaderInputListForImageUnboundedArrays())
            {
                const RHI::ShaderInputImageUnboundedArrayIndex imageUnboundedArrayInputIndex(shaderInputIndex);
                AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> imageViews = groupData.GetImageViewUnboundedArray(imageUnboundedArrayInputIndex);

                uint32_t tableIndex = shaderInputIndex * RHI::Limits::Device::FrameCountMax + group.m_compiledDataIndex;

                // resize the descriptor table allocation if necessary
                if (group.m_unboundedDescriptorTables[tableIndex].GetSize() != imageViews.size())
                {
                   
                    if (group.m_unboundedDescriptorTables[tableIndex].IsValid())
                    {
                        m_descriptorContext->ReleaseDescriptorTable(group.m_unboundedDescriptorTables[tableIndex], &group);
                        group.m_unboundedDescriptorTables[tableIndex] = DescriptorTable{};
                    }

                    if (!imageViews.empty())
                    {
                        group.m_unboundedDescriptorTables[tableIndex] = m_descriptorContext->CreateDescriptorTable(
                            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(imageViews.size()), &group);

                        if (!group.m_unboundedDescriptorTables[tableIndex].IsValid())
                        {
                            // We have support for compacting D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV (if applicable) so try that
                            device.DescriptorHeapCompactionNeeded();
                            return;
                        }

                        ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[group.m_compiledDataIndex];
                        compiledData.m_gpuUnboundedArraysDescriptorHandles[shaderInputIndex] = m_descriptorContext->GetGpuPlatformHandleForTable(group.m_unboundedDescriptorTables[tableIndex]);
                    }
                }

                const DescriptorTable descriptorTable(group.m_unboundedDescriptorTables[tableIndex].GetOffset(), static_cast<uint16_t>(imageViews.size()));
                UpdateUnboundedImagesDescTable(descriptorTable, groupData, shaderInputIndex, shaderInputImageUnboundedArray.m_access, shaderInputImageUnboundedArray.m_type);
                ++shaderInputIndex;
            }
        }

        void ShaderResourceGroupPool::UpdateUnboundedBuffersDescTable(
            DescriptorTable descriptorTable,
            const RHI::ShaderResourceGroupData& groupData,
            uint32_t shaderInputIndex,
            RHI::ShaderInputBufferAccess bufferAccess)
        {
            const RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayInputIndex(shaderInputIndex);
            AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> bufferViews =
                groupData.GetBufferViewUnboundedArray(bufferUnboundedArrayInputIndex);

            if (bufferViews.empty())
            {
                // we don't need to update descriptors since the buffer list is empty
                return;
            }

            D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType = ConvertShaderInputBufferAccess(bufferAccess);

            AZStd::vector<DescriptorHandle> descriptorHandles;
            switch (descriptorRangeType)
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                {
                    descriptorHandles = GetSRVsFromImageViews<RHI::BufferView, BufferView>(bufferViews, D3D12_SRV_DIMENSION_BUFFER);
                    break;
                }
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                {
                    descriptorHandles = GetUAVsFromImageViews<RHI::BufferView, BufferView>(bufferViews, D3D12_UAV_DIMENSION_BUFFER);
                    break;
                }
            default:
                AZ_Assert(false, "Unhandled D3D12_DESCRIPTOR_RANGE_TYPE enumeration");
                break;
            }

            m_descriptorContext->UpdateDescriptorTableRange(
                descriptorTable, descriptorHandles.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void ShaderResourceGroupPool::UpdateUnboundedImagesDescTable(
            DescriptorTable descriptorTable,
            const RHI::ShaderResourceGroupData& groupData,
            uint32_t shaderInputIndex,
            RHI::ShaderInputImageAccess imageAccess,
            RHI::ShaderInputImageType imageType)
        {
            const RHI::ShaderInputImageUnboundedArrayIndex imageUnboundedArrayInputIndex(shaderInputIndex);
            AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> imageViews =
                groupData.GetImageViewUnboundedArray(imageUnboundedArrayInputIndex);

            if (imageViews.empty())
            {
                // we don't need to update descriptors since the image list is empty
                return;
            }

            D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType = ConvertShaderInputImageAccess(imageAccess);

            AZStd::vector<DescriptorHandle> descriptorHandles;
            switch (descriptorRangeType)
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                {
                    descriptorHandles = GetSRVsFromImageViews<RHI::ImageView, ImageView>(imageViews, ConvertSRVDimension(imageType));
                    break;
                }
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                {
                    descriptorHandles = GetUAVsFromImageViews<RHI::ImageView, ImageView>(imageViews, ConvertUAVDimension(imageType));
                    break;
                }
            default:
                AZ_Assert(false, "Unhandled D3D12_DESCRIPTOR_RANGE_TYPE enumeration");
                break;
            }

            m_descriptorContext->UpdateDescriptorTableRange(
                descriptorTable, descriptorHandles.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        RHI::ResultCode ShaderResourceGroupPool::UpdateDescriptorTableAfterCompaction(
            RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData)
        {
            // Since we are trying to compact we will re-create all the descriptor tables and re-update them all
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);
             
            if (m_viewsDescriptorTableSize)
            {
                group.m_viewsDescriptorTable = m_descriptorContext->CreateDescriptorTable(
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_viewsDescriptorTableRingSize, &group);

                if (!group.m_viewsDescriptorTable.IsValid())
                {
                    AZ_Assert(
                        false,
                        "Descriptor heap ran out of memory. Please consider increasing number of handles allowed for the second value"
                        "of DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV within platformlimits.azasset file for dx12.");
                    return RHI::ResultCode::OutOfMemory;
                }

                CacheGpuHandlesForViews(group);

                const DescriptorTable descriptorTable(
                    group.m_viewsDescriptorTable.GetOffset() + group.m_compiledDataIndex * m_viewsDescriptorTableSize,
                        static_cast<uint16_t>(m_viewsDescriptorTableSize));

                UpdateViewsDescriptorTable(descriptorTable, groupData);
            }

            if (m_unboundedArrayCount)
            {
                //Reset all the old descriptor tables as the previous heap is gone. 
                for (uint32_t unboundedArrayindex = 0; unboundedArrayindex < (ShaderResourceGroupCompiledData::MaxUnboundedArrays * RHI::Limits::Device::FrameCountMax); ++unboundedArrayindex)
                {
                    group.m_unboundedDescriptorTables[unboundedArrayindex] = DescriptorTable{};
                }

                const RHI::ShaderResourceGroupLayout& groupLayout = *groupData.GetLayout();
                uint32_t shaderInputIndex = 0;

                // process buffer unbounded arrays
                for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray : groupLayout.GetShaderInputListForBufferUnboundedArrays())
                {
                    const RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayInputIndex(shaderInputIndex);
                    AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> bufferViews = groupData.GetBufferViewUnboundedArray(bufferUnboundedArrayInputIndex);

                    uint32_t tableIndex = shaderInputIndex * RHI::Limits::Device::FrameCountMax + group.m_compiledDataIndex;
                    if (!bufferViews.empty())
                    {
                        group.m_unboundedDescriptorTables[tableIndex] = m_descriptorContext->CreateDescriptorTable(
                            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(bufferViews.size()), &group);

                        if (!group.m_unboundedDescriptorTables[tableIndex].IsValid())
                        {
                            AZ_Assert(
                                false,
                                "Descriptor heap ran out of memory. Please consider increasing number of handles allowed for the second value"
                                "of DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV within platformlimits.azasset file for dx12.");
                            return RHI::ResultCode::OutOfMemory;
                        }

                        ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[group.m_compiledDataIndex];
                        compiledData.m_gpuUnboundedArraysDescriptorHandles[shaderInputIndex] = m_descriptorContext->GetGpuPlatformHandleForTable(group.m_unboundedDescriptorTables[tableIndex]);

                        const DescriptorTable descriptorTable(
                            group.m_unboundedDescriptorTables[tableIndex].GetOffset(), static_cast<uint16_t>(bufferViews.size()));
                        UpdateUnboundedBuffersDescTable(descriptorTable, groupData, shaderInputIndex, shaderInputBufferUnboundedArray.m_access);
                    }
                    shaderInputIndex++;
                }

                // process image unbounded arrays
                for (const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray :
                     groupLayout.GetShaderInputListForImageUnboundedArrays())
                {
                    const RHI::ShaderInputImageUnboundedArrayIndex imageUnboundedArrayInputIndex(shaderInputIndex);
                    AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> imageViews =
                        groupData.GetImageViewUnboundedArray(imageUnboundedArrayInputIndex);

                    uint32_t tableIndex = shaderInputIndex * RHI::Limits::Device::FrameCountMax + group.m_compiledDataIndex;
                    if (!imageViews.empty())
                    {
                        group.m_unboundedDescriptorTables[tableIndex] = m_descriptorContext->CreateDescriptorTable(
                            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_cast<uint32_t>(imageViews.size()), &group);

                        if (!group.m_unboundedDescriptorTables[tableIndex].IsValid())
                        {
                            AZ_Assert(
                                false,
                                "Descriptor heap ran out of memory. Please consider increasing number of handles allowed for the second value"
                                "of DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV within platformlimits.azasset file for dx12.");
                            return RHI::ResultCode::OutOfMemory;
                        }

                        ShaderResourceGroupCompiledData& compiledData = group.m_compiledData[group.m_compiledDataIndex];
                        compiledData.m_gpuUnboundedArraysDescriptorHandles[shaderInputIndex] = m_descriptorContext->GetGpuPlatformHandleForTable(group.m_unboundedDescriptorTables[tableIndex]);

                        const DescriptorTable descriptorTable(group.m_unboundedDescriptorTables[tableIndex].GetOffset(), static_cast<uint16_t>(imageViews.size()));
                        UpdateUnboundedImagesDescTable(descriptorTable, groupData, shaderInputIndex, shaderInputImageUnboundedArray.m_access, shaderInputImageUnboundedArray.m_type);
                    }
                    shaderInputIndex++;
                }
            }
            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::OnFrameEnd()
        {
            m_constantAllocator.GarbageCollect();
            Base::OnFrameEnd();
        }

        DescriptorTable ShaderResourceGroupPool::GetBufferTable(DescriptorTable descriptorTable, RHI::ShaderInputBufferIndex bufferInputIndex) const
        {
            const RHI::Interval interval = GetLayout()->GetGroupInterval(bufferInputIndex);
            const DescriptorHandle startHandle = descriptorTable[m_descriptorTableBufferOffset + interval.m_min];
            return DescriptorTable(startHandle, aznumeric_caster(interval.m_max - interval.m_min));
        }

        DescriptorTable ShaderResourceGroupPool::GetImageTable(DescriptorTable descriptorTable, RHI::ShaderInputImageIndex imageInputIndex) const
        {
            const RHI::Interval interval = GetLayout()->GetGroupInterval(imageInputIndex);
            const DescriptorHandle startHandle = descriptorTable[m_descriptorTableImageOffset + interval.m_min];
            return DescriptorTable(startHandle, aznumeric_caster(interval.m_max - interval.m_min));
        }

        DescriptorTable ShaderResourceGroupPool::GetSamplerTable(DescriptorTable descriptorTable, RHI::ShaderInputSamplerIndex samplerInputIndex) const
        {
            const RHI::Interval interval = GetLayout()->GetGroupInterval(samplerInputIndex);
            const DescriptorHandle startHandle = descriptorTable[interval.m_min];
            return DescriptorTable(startHandle, aznumeric_caster(interval.m_max - interval.m_min));
        }

        void ShaderResourceGroupPool::UpdateDescriptorTableRange(
            DescriptorTable descriptorTable,
            const AZStd::vector<DescriptorHandle>& handles,
            RHI::ShaderInputBufferIndex bufferInputIndex)
        {
            const DescriptorTable gpuDestinationTable = GetBufferTable(descriptorTable, bufferInputIndex);
            m_descriptorContext->UpdateDescriptorTableRange(gpuDestinationTable, handles.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void ShaderResourceGroupPool::UpdateDescriptorTableRange(
            DescriptorTable descriptorTable,
            const AZStd::vector<DescriptorHandle>& handles,
            RHI::ShaderInputImageIndex imageInputIndex)
        {
            const DescriptorTable gpuDestinationTable = GetImageTable(descriptorTable, imageInputIndex);
            m_descriptorContext->UpdateDescriptorTableRange(gpuDestinationTable, handles.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void ShaderResourceGroupPool::UpdateDescriptorTableRange(
            DescriptorTable descriptorTable,
            RHI::ShaderInputSamplerIndex samplerInputIndex, 
            AZStd::array_view<RHI::SamplerState> samplerStates)
        {
            const DescriptorHandle nullHandle = m_descriptorContext->GetNullHandleSampler();
            AZStd::vector<DescriptorHandle> cpuSourceDescriptors(aznumeric_caster(samplerStates.size()), nullHandle);
            auto& device = static_cast<Device&>(GetDevice());
            AZStd::vector<RHI::ConstPtr<Sampler>> samplers(samplerStates.size(), nullptr);
            for (size_t i = 0; i < samplerStates.size(); ++i)
            {
                samplers[i] = device.AcquireSampler(samplerStates[i]);
                cpuSourceDescriptors[i] = samplers[i]->GetDescriptorHandle();
            }

            const DescriptorTable gpuDestinationTable = GetSamplerTable(descriptorTable, samplerInputIndex);
            m_descriptorContext->UpdateDescriptorTableRange(gpuDestinationTable, cpuSourceDescriptors.data(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }
    }
}

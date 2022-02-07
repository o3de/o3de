/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Conversion.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/DescriptorSet.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/MemoryView.h>
#include <RHI/NullDescriptorManager.h>

namespace AZ
{
    namespace Vulkan
    {
        const DescriptorSet::Descriptor& DescriptorSet::GetDescriptor() const
        {
            return m_descriptor;
        }

        VkDescriptorSet DescriptorSet::GetNativeDescriptorSet() const
        {
            return m_nativeDescriptorSet;
        }

        void DescriptorSet::CommitUpdates()
        {
            if (!m_updateData.empty())
            {
                UpdateNativeDescriptorSet();
            }            
        }

        void DescriptorSet::UpdateBufferViews(uint32_t layoutIndex, const AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>& bufViews)
        {
            const DescriptorSetLayout& layout = *m_descriptor.m_descriptorSetLayout;
            VkDescriptorType type = layout.GetDescriptorType(layoutIndex);
            
            WriteDescriptorData data;
            data.m_layoutIndex = layoutIndex;

            if (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
                type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
            {
                data.m_texelBufferViews.resize(bufViews.size());
                for (size_t i = 0; i < bufViews.size(); ++i)
                {
                    const RHI::ConstPtr<RHI::BufferView>& bufferView = bufViews[i];
                    VkBufferView vkBufferView;
                    if (!bufferView || bufferView->IsStale())
                    {
                        if (m_nullDescriptorSupported)
                        {
                            vkBufferView = VK_NULL_HANDLE;
                        }
                        else
                        {
                            auto& device = static_cast<Device&>(GetDevice());
                            vkBufferView = device.GetNullDescriptorManager().GetTexelBufferView();
                        }
                    }
                    else
                    {
                        vkBufferView = static_cast<const BufferView&>(*bufferView.get()).GetNativeTexelBufferView();
                    }

                    data.m_texelBufferViews[i] = vkBufferView;
                }
            }
            else
            {
                data.m_bufferViewsInfo.resize(bufViews.size());
                data.m_accelerationStructures.resize(bufViews.size());
                for (size_t i = 0; i < bufViews.size(); ++i)
                {
                    VkDescriptorBufferInfo bufferInfo = {};
                    const RHI::ConstPtr<RHI::BufferView>& bufferView = bufViews[i];
                    if (!bufferView || bufferView->IsStale())
                    {
                        if (m_nullDescriptorSupported)
                        {
                            bufferInfo.buffer = VK_NULL_HANDLE;
                            bufferInfo.offset = 0;
                            bufferInfo.range = VK_WHOLE_SIZE;
                        }
                        else
                        {
                            auto& device = static_cast<Device&>(GetDevice());
                            bufferInfo = device.GetNullDescriptorManager().GetBuffer();
                        }
                    }
                    else
                    {
                        auto& bufferViewDescriptor = bufferView->GetDescriptor();
                        const BufferMemoryView* bufferMemoryView = static_cast<const Buffer&>(bufferView->GetBuffer()).GetBufferMemoryView();
                        bufferInfo.buffer = bufferMemoryView->GetNativeBuffer();
                        bufferInfo.offset = bufferMemoryView->GetOffset() + bufferViewDescriptor.m_elementOffset * bufferViewDescriptor.m_elementSize;
                        bufferInfo.range = bufferViewDescriptor.m_elementCount * bufferViewDescriptor.m_elementSize;
                    }

                    data.m_bufferViewsInfo[i] = bufferInfo;
                    
                    // if this is a buffer view of a RayTracingTLAS we need to store the vkAccelerationStructureKHR with it
                    if (bufferView && (type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR))
                    {
                        data.m_accelerationStructures[i] = static_cast<const BufferView&>(*bufferView.get()).GetNativeAccelerationStructure();
                    }
                }
            }

            m_updateData.push_back(AZStd::move(data));
        }

        void DescriptorSet::UpdateImageViews(uint32_t layoutIndex, const AZStd::array_view<RHI::ConstPtr<RHI::ImageView>>& imageViews, RHI::ShaderInputImageType imageType)
        {
            const DescriptorSetLayout& layout = *m_descriptor.m_descriptorSetLayout;

            WriteDescriptorData data;
            data.m_layoutIndex = layoutIndex;

            data.m_imageViewsInfo.resize(imageViews.size());
            for (size_t i = 0; i < imageViews.size(); ++i)
            {
                VkDescriptorImageInfo imageInfo = {};
                const auto* imageView = static_cast<const ImageView*>(imageViews[i].get());
                if (!imageView || imageView->IsStale())
                {
                    if (m_nullDescriptorSupported)
                    {
                        imageInfo.imageView = VK_NULL_HANDLE;
                    }
                    else
                    {
                        auto& device = static_cast<Device&>(GetDevice());
                        NullDescriptorManager& nullDescriptorManager = device.GetNullDescriptorManager();
                        bool storageImage = (layout.GetDescriptorType(layoutIndex) == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                        imageInfo = nullDescriptorManager.GetDescriptorImageInfo(imageType, storageImage);
                    }
                }
                else
                {
                    imageInfo.imageView = imageView->GetNativeImageView();

                    // Depending on the access (read or readwrite) and if it's a depth/stencil image, we choose the expected layout.
                    switch (layout.GetDescriptorType(layoutIndex))
                    {
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        break;
                    default:
                        imageInfo.imageLayout = RHI::CheckBitsAny(imageView->GetImage().GetAspectFlags(), RHI::ImageAspectFlags::DepthStencil) ?
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        break;
                    }
                }
                
                data.m_imageViewsInfo[i]  = imageInfo;
            }

            m_updateData.push_back(AZStd::move(data));
        }

        void DescriptorSet::UpdateSamplers(uint32_t layoutIndex, const AZStd::array_view<RHI::SamplerState>& samplers)
        {
            auto& device = static_cast<Device&>(GetDevice());

            WriteDescriptorData data;
            data.m_layoutIndex = layoutIndex;

            VkDescriptorImageInfo imageInfo = {};
            for (const RHI::SamplerState& samplerState : samplers)
            {
                Sampler::Descriptor samplerDesc;
                samplerDesc.m_device = &device;
                samplerDesc.m_samplerState = samplerState;
                imageInfo.sampler = device.AcquireSampler(samplerDesc)->GetNativeSampler();
                data.m_imageViewsInfo.push_back(imageInfo);
            }

            m_updateData.push_back(AZStd::move(data));
        }

        void DescriptorSet::UpdateConstantData(AZStd::array_view<uint8_t> rawData)
        {
            AZ_Assert(m_constantDataBuffer, "Null constant buffer");
            const DescriptorSetLayout& layout = *m_descriptor.m_descriptorSetLayout;

            BufferMemoryView* memoryView = m_constantDataBuffer->GetBufferMemoryView();
            uint8_t* mappedData = static_cast<uint8_t*>(memoryView->Map(RHI::HostMemoryAccess::Write));
            memcpy(mappedData, rawData.data(), rawData.size());
            memoryView->Unmap(RHI::HostMemoryAccess::Write);

            WriteDescriptorData data;
            data.m_layoutIndex = layout.GetLayoutIndexFromGroupIndex(0, DescriptorSetLayout::ResourceType::ConstantData);

            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = memoryView->GetNativeBuffer();
            bufferInfo.offset = memoryView->GetOffset();
            bufferInfo.range = rawData.size();
            data.m_bufferViewsInfo.push_back(bufferInfo);
            m_updateData.push_back(AZStd::move(data));
        }

        RHI::Ptr<DescriptorSet> DescriptorSet::Create()
        {
            return aznew DescriptorSet();
        }

        VkResult DescriptorSet::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(descriptor.m_device, "Device is null.");
            AZ_Assert(descriptor.m_descriptorPool, "DescriptorPool is null.");
            AZ_Assert(descriptor.m_descriptorSetLayout, "DescriptorSetLayout is null.");
            Base::Init(*descriptor.m_device);

            // if this descriptor set contains an unbounded array we need to defer allocation until UpdateNativeDescriptorSet(),
            // since we do not know the number of views in the unbounded array
            if (!descriptor.m_descriptorSetLayout->GetHasUnboundedArray())
            {
                VkDescriptorSetLayout nativeLayout = descriptor.m_descriptorSetLayout->GetNativeDescriptorSetLayout();
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = descriptor.m_descriptorPool->GetNativeDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &nativeLayout;

                VkResult result = vkAllocateDescriptorSets(descriptor.m_device->GetNativeDevice(), &allocInfo, &m_nativeDescriptorSet);
                if (result == VK_ERROR_FRAGMENTED_POOL)
                {
                    // fragmented pool will be re-created subsequently in DescriptorSetAllocator, so warning only 
                    AZ_Warning("Vulkan RHI", false, "Fragmented pool, will be recreated in DescriptorSetAllocator afterward");
                }
                else 
                {
                    AssertSuccess(result);
                }

                if (result != VK_SUCCESS)
                {
                    return result;
                }
            }

            // Check if we need to create a uniform buffer for the constants
            DescriptorPool::Descriptor const& vulkanDescriptor = m_descriptor.m_descriptorPool->GetDescriptor();
            size_t constantDataSize = m_descriptor.m_descriptorSetLayout->GetConstantDataSize();
            if (vulkanDescriptor.m_constantDataPool && constantDataSize)
            {
                m_constantDataBuffer = Buffer::Create();
                const RHI::BufferDescriptor bufferDescriptor(RHI::BufferBindFlags::Constant, constantDataSize);
                RHI::BufferInitRequest request(*m_constantDataBuffer, bufferDescriptor);
                RHI::ResultCode rhiResult = vulkanDescriptor.m_constantDataPool->InitBuffer(request);
                if (rhiResult != RHI::ResultCode::Success)
                {
                    return VK_ERROR_OUT_OF_HOST_MEMORY;
                }

                auto bufferView = m_constantDataBuffer->GetBufferView(
                    RHI::BufferViewDescriptor::CreateStructured(
                        0,
                        1,
                        aznumeric_caster(constantDataSize)));
                m_constantDataBufferView = AZStd::static_pointer_cast<BufferView>(bufferView);
            }

            m_nullDescriptorSupported = static_cast<const PhysicalDevice&>(m_descriptor.m_device->GetPhysicalDevice()).IsFeatureSupported(DeviceFeature::NullDescriptor);

            SetName(GetName());
            return VK_SUCCESS;
        }

        void DescriptorSet::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty() && m_nativeDescriptorSet != VK_NULL_HANDLE)
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeDescriptorSet), name.data(), VK_OBJECT_TYPE_DESCRIPTOR_SET, static_cast<Device&>(GetDevice()));
            }
        }

        void DescriptorSet::Shutdown()
        {
            if (m_nativeDescriptorSet != VK_NULL_HANDLE)
            {
                AZ_Assert(m_descriptor.m_descriptorPool, "Descriptor pool is null.");
                auto& device = static_cast<Device&>(GetDevice());
                AssertSuccess(vkFreeDescriptorSets(device.GetNativeDevice(), m_descriptor.m_descriptorPool->GetNativeDescriptorPool(), 1, &m_nativeDescriptorSet));
                m_nativeDescriptorSet = VK_NULL_HANDLE;
            }
            m_constantDataBufferView = nullptr;
            m_constantDataBuffer = nullptr;
            Base::Shutdown();
        }

        void DescriptorSet::UpdateNativeDescriptorSet()
        {
            // if this descriptor set has an unbounded array we need to allocate it now, or if it
            // is already allocated adjust the allocation size
            if (m_descriptor.m_descriptorSetLayout->GetHasUnboundedArray())
            {
                AllocateDescriptorSetWithUnboundedArray();
            }

            AZStd::vector<VkWriteDescriptorSet> writeDescSetDescs;
            AZStd::vector<VkWriteDescriptorSetAccelerationStructureKHR> writeAccelerationStructureDescs;
            const DescriptorSetLayout& layout = *m_descriptor.m_descriptorSetLayout;
            for (const WriteDescriptorData& updateData : m_updateData)
            {
                const VkDescriptorType descType = layout.GetDescriptorType(updateData.m_layoutIndex);

                VkWriteDescriptorSet writeDescSet{};
                writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescSet.pNext = nullptr;
                writeDescSet.dstSet = m_nativeDescriptorSet;
                writeDescSet.dstBinding = layout.GetBindingIndex(updateData.m_layoutIndex);
                writeDescSet.descriptorType = descType;

                switch (descType)
                {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    AZ_Assert(!updateData.m_bufferViewsInfo.empty(), "BufferInfo is empty.");
                    for (const RHI::Interval& interval : GetValidDescriptorsIntervals(updateData.m_bufferViewsInfo))
                    {
                        writeDescSet.pBufferInfo = updateData.m_bufferViewsInfo.data() + interval.m_min;
                        writeDescSet.dstArrayElement = interval.m_min;
                        writeDescSet.descriptorCount = interval.m_max - interval.m_min;
                        writeDescSetDescs.push_back(AZStd::move(writeDescSet));
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    AZ_Assert(!updateData.m_imageViewsInfo.empty(), "ImageInfo is empty.");
                    for (const RHI::Interval& interval : GetValidDescriptorsIntervals(updateData.m_imageViewsInfo))
                    {
                        writeDescSet.pImageInfo = updateData.m_imageViewsInfo.data() + interval.m_min;
                        writeDescSet.dstArrayElement = interval.m_min;
                        writeDescSet.descriptorCount = interval.m_max - interval.m_min;
                        writeDescSetDescs.push_back(AZStd::move(writeDescSet));
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    AZ_Assert(!updateData.m_texelBufferViews.empty(), "TexelInfo list is empty.");
                    for (const RHI::Interval& interval : GetValidDescriptorsIntervals(updateData.m_texelBufferViews))
                    {
                        writeDescSet.pTexelBufferView = updateData.m_texelBufferViews.data() + interval.m_min;
                        writeDescSet.dstArrayElement = interval.m_min;
                        writeDescSet.descriptorCount = interval.m_max - interval.m_min;
                        writeDescSetDescs.push_back(AZStd::move(writeDescSet));
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    AZ_Assert(!updateData.m_bufferViewsInfo.empty(), "BufferInfo is empty.");
                    AZ_Assert(!updateData.m_accelerationStructures.empty(), "AccelerationStructures is empty.");
                    for (const RHI::Interval& interval : GetValidDescriptorsIntervals(updateData.m_bufferViewsInfo))
                    {
                        // acceleration structure descriptor is added as the pNext in the VkWriteDescriptorSet
                        VkWriteDescriptorSetAccelerationStructureKHR writeAccelerationStructure = {};
                        writeAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                        writeAccelerationStructure.accelerationStructureCount = 1;
                        writeAccelerationStructure.pAccelerationStructures = updateData.m_accelerationStructures.data() + interval.m_min;
                        writeAccelerationStructureDescs.push_back(AZStd::move(writeAccelerationStructure));

                        writeDescSet.dstArrayElement = interval.m_min;
                        writeDescSet.descriptorCount = interval.m_max - interval.m_min;
                        writeDescSet.pNext = &writeAccelerationStructureDescs.back();
                        writeDescSetDescs.push_back(AZStd::move(writeDescSet));
                    }
                    break;
                default:
                    AZ_Assert(false, "Unsupported descriptor type %d.", descType);
                    break;
                }
            }

            if (!writeDescSetDescs.empty())
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkUpdateDescriptorSets(device.GetNativeDevice(), static_cast<uint32_t>(writeDescSetDescs.size()), writeDescSetDescs.data(), 0, nullptr);
            }

            m_updateData.clear();
        }

        void DescriptorSet::AllocateDescriptorSetWithUnboundedArray()
        {
            const DescriptorSetLayout& layout = *m_descriptor.m_descriptorSetLayout;
            uint32_t unboundedArraySize = 0;

            // find the unbounded array in the update data
            for (const WriteDescriptorData& updateData : m_updateData)
            {
                if ((layout.GetNativeBindingFlags()[updateData.m_layoutIndex] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) != 0)
                {
                    // this is the unbounded array, find the size
                    const VkDescriptorType descType = layout.GetDescriptorType(updateData.m_layoutIndex);

                    switch (descType)
                    {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        unboundedArraySize = aznumeric_cast<uint32_t>(updateData.m_bufferViewsInfo.size());
                        break;
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        unboundedArraySize = aznumeric_cast<uint32_t>(updateData.m_imageViewsInfo.size());
                        break;
                    default:
                        AZ_Assert(false, "Unsupported descriptor type for unbounded array");
                        return;
                    }

                    if (unboundedArraySize != m_currentUnboundedArrayAllocation)
                    {
                        // release existing descriptor set if necessary
                        if (m_nativeDescriptorSet)
                        {
                            AssertSuccess(vkFreeDescriptorSets(m_descriptor.m_device->GetNativeDevice(), m_descriptor.m_descriptorPool->GetNativeDescriptorPool(), 1, &m_nativeDescriptorSet));
                            m_nativeDescriptorSet = VK_NULL_HANDLE;
                        }
                    }

                    break;
                }
            }

            if (m_nativeDescriptorSet == VK_NULL_HANDLE)
            {
                VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCounts = {};
                variableDescriptorCounts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                variableDescriptorCounts.descriptorSetCount = 1;
                variableDescriptorCounts.pDescriptorCounts = &unboundedArraySize;

                VkDescriptorSetLayout nativeLayout = m_descriptor.m_descriptorSetLayout->GetNativeDescriptorSetLayout();
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.pNext = &variableDescriptorCounts;
                allocInfo.descriptorPool = m_descriptor.m_descriptorPool->GetNativeDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &nativeLayout;

                AssertSuccess(vkAllocateDescriptorSets(m_descriptor.m_device->GetNativeDevice(), &allocInfo, &m_nativeDescriptorSet));

                m_currentUnboundedArrayAllocation = unboundedArraySize;
                SetName(GetName());
            }
        }

        bool DescriptorSet::IsNullDescriptorInfo(const VkDescriptorBufferInfo& descriptorInfo)
        {
            return descriptorInfo.buffer == VK_NULL_HANDLE;
        }

        bool DescriptorSet::IsNullDescriptorInfo(const VkDescriptorImageInfo& descriptorInfo)
        {
            return (descriptorInfo.imageView == VK_NULL_HANDLE && descriptorInfo.sampler == VK_NULL_HANDLE);
        }

        bool DescriptorSet::IsNullDescriptorInfo(const VkBufferView& descriptorInfo)
        {
            return descriptorInfo == VK_NULL_HANDLE;
        }

        RHI::Ptr<BufferView> DescriptorSet::GetConstantDataBufferView() const
        {
            return m_constantDataBufferView;
        }
   }
}

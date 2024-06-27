/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom_RHI_Vulkan_Platform.h>
#include <RHI/BindlessDescriptorPool.h>
#include <RHI/BufferView.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceShaderResourceGroupData.h>


namespace AZ::Vulkan
{
    
    RHI::ResultCode BindlessDescriptorPool::Init(Device& device, const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc)
    {
        m_device = &device;

        const uint32_t MaxBindlessIndices = static_cast<uint32_t>(AZ::RHI::BindlessResourceType::Count);
        m_bindlessSrgDesc = bindlessSrgDesc;

        {
            DescriptorPool::Descriptor desc;
            desc.m_device = &device;
            
            desc.m_descriptorPoolSizes.resize_no_construct(MaxBindlessIndices);
            desc.m_descriptorPoolSizes[m_bindlessSrgDesc.m_roTextureIndex] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RHI::Limits::Pipeline::UnboundedArraySize };
            desc.m_descriptorPoolSizes[m_bindlessSrgDesc.m_rwTextureIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RHI::Limits::Pipeline::UnboundedArraySize };
            desc.m_descriptorPoolSizes[m_bindlessSrgDesc.m_roBufferIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RHI::Limits::Pipeline::UnboundedArraySize };
            desc.m_descriptorPoolSizes[m_bindlessSrgDesc.m_rwBufferIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RHI::Limits::Pipeline::UnboundedArraySize };
            desc.m_descriptorPoolSizes[m_bindlessSrgDesc.m_roTextureCubeIndex] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RHI::Limits::Pipeline::UnboundedArraySize };
            desc.m_maxSets = 1;
            desc.m_collectLatency = 1;
            desc.m_updateAfterBind = true;

            m_pool = DescriptorPool::Create();
            m_pool->Init(desc);
        }

        {
            VkDescriptorSetLayoutBinding bindings[MaxBindlessIndices];
      
            bindings[m_bindlessSrgDesc.m_roTextureIndex].binding = m_bindlessSrgDesc.m_roTextureIndex;
            bindings[m_bindlessSrgDesc.m_roTextureIndex].descriptorCount = RHI::Limits::Pipeline::UnboundedArraySize;
            bindings[m_bindlessSrgDesc.m_roTextureIndex].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindings[m_bindlessSrgDesc.m_roTextureIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[m_bindlessSrgDesc.m_rwTextureIndex].binding = m_bindlessSrgDesc.m_rwTextureIndex;
            bindings[m_bindlessSrgDesc.m_rwTextureIndex].descriptorCount = RHI::Limits::Pipeline::UnboundedArraySize;
            bindings[m_bindlessSrgDesc.m_rwTextureIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            bindings[m_bindlessSrgDesc.m_rwTextureIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[m_bindlessSrgDesc.m_roBufferIndex].binding = m_bindlessSrgDesc.m_roBufferIndex;
            bindings[m_bindlessSrgDesc.m_roBufferIndex].descriptorCount = RHI::Limits::Pipeline::UnboundedArraySize;
            bindings[m_bindlessSrgDesc.m_roBufferIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[m_bindlessSrgDesc.m_roBufferIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[m_bindlessSrgDesc.m_rwBufferIndex].binding = m_bindlessSrgDesc.m_rwBufferIndex;
            bindings[m_bindlessSrgDesc.m_rwBufferIndex].descriptorCount = RHI::Limits::Pipeline::UnboundedArraySize;
            bindings[m_bindlessSrgDesc.m_rwBufferIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[m_bindlessSrgDesc.m_rwBufferIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[m_bindlessSrgDesc.m_roTextureCubeIndex].binding = m_bindlessSrgDesc.m_roTextureCubeIndex;
            bindings[m_bindlessSrgDesc.m_roTextureCubeIndex].descriptorCount = RHI::Limits::Pipeline::UnboundedArraySize;
            bindings[m_bindlessSrgDesc.m_roTextureCubeIndex].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindings[m_bindlessSrgDesc.m_roTextureCubeIndex].stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorBindingFlags bindingFlags[MaxBindlessIndices];
            for (size_t i = 0; i != MaxBindlessIndices; ++i)
            {
                bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            }
            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingFlagsInfo.bindingCount = MaxBindlessIndices;
            bindingFlagsInfo.pBindingFlags = bindingFlags;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = &bindingFlagsInfo;
            layoutInfo.bindingCount = MaxBindlessIndices;
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            layoutInfo.pBindings = bindings;

            VkResult result = m_device->GetContext().CreateDescriptorSetLayout(
                m_device->GetNativeDevice(), &layoutInfo, VkSystemAllocator::Get(), &m_descriptorSetLayout);
            if (result != VK_SUCCESS)
            {
                AssertSuccess(result);
                return ConvertResult(result);
            }

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_pool->GetNativeDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_descriptorSetLayout;

            result = m_device->GetContext().AllocateDescriptorSets(m_device->GetNativeDevice(), &allocInfo, &m_set);
            if (result != VK_SUCCESS)
            {
                AssertSuccess(result);
                return ConvertResult(result);
            }
        }

        for (size_t i = 0; i != MaxBindlessIndices; ++i)
        {
            RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = RHI::Limits::Pipeline::UnboundedArraySize;
            desc.m_alignmentInBytes = 1;
            desc.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            desc.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocators[i].Init(desc);
        }

        return RHI::ResultCode::Success;
    }

    void BindlessDescriptorPool::Shutdown()
    {
        m_device->GetContext().FreeDescriptorSets(m_device->GetNativeDevice(), m_pool->GetNativeDescriptorPool(), 1, &m_set);
        m_device->GetContext().DestroyDescriptorSetLayout(m_device->GetNativeDevice(), m_descriptorSetLayout, VkSystemAllocator::Get());

        m_pool.reset();
    }

    VkWriteDescriptorSet BindlessDescriptorPool::PrepareWrite(uint32_t index, uint32_t binding, VkDescriptorType type)
    {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_set;
        write.dstBinding = binding;
        write.descriptorType = type;
        write.dstArrayElement = index;
        write.descriptorCount = 1;

        return write;
    }

    uint32_t BindlessDescriptorPool::AttachReadImage(ImageView* view)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = view->GetBindlessReadIndex();
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roTextureIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        VkWriteDescriptorSet write = PrepareWrite(heapIndex, m_bindlessSrgDesc.m_roTextureIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = RHI::CheckBitsAny(view->GetImage().GetAspectFlags(), RHI::ImageAspectFlags::DepthStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfo.imageView = view->GetNativeImageView();
        write.pImageInfo = &imageInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);
        return heapIndex;
    }

    uint32_t BindlessDescriptorPool::AttachReadWriteImage(ImageView* view)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = view->GetBindlessReadWriteIndex();
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_rwTextureIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        VkWriteDescriptorSet write = PrepareWrite(heapIndex, m_bindlessSrgDesc.m_rwTextureIndex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = RHI::CheckBitsAny(view->GetImage().GetAspectFlags(), RHI::ImageAspectFlags::DepthStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_GENERAL;

        imageInfo.imageView = view->GetNativeImageView();
        write.pImageInfo = &imageInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return heapIndex;
    }

    uint32_t BindlessDescriptorPool::AttachReadBuffer(BufferView* view)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = view->GetBindlessReadIndex();
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roBufferIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();
        VkWriteDescriptorSet write = PrepareWrite(heapIndex, m_bindlessSrgDesc.m_roBufferIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return heapIndex;
    }

    uint32_t BindlessDescriptorPool::AttachReadWriteBuffer(BufferView* view)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = view->GetBindlessReadWriteIndex();
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_rwBufferIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();
        VkWriteDescriptorSet write = PrepareWrite(heapIndex, m_bindlessSrgDesc.m_rwBufferIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);
        
        return heapIndex;
    }

    uint32_t BindlessDescriptorPool::AttachReadCubeMapImage(ImageView* view)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = view->GetBindlessReadIndex();
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roTextureCubeIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        VkWriteDescriptorSet write = PrepareWrite(heapIndex, m_bindlessSrgDesc.m_roTextureCubeIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = view->GetNativeImageView();

        write.pImageInfo = &imageInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);
        return heapIndex;
    }

    void BindlessDescriptorPool::DetachReadImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roTextureIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_rwTextureIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roBufferIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_rwBufferIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadCubeMapImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roTextureCubeIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::GarbageCollect()
    {
        for (size_t i = 0; i != static_cast<uint32_t>(AZ::RHI::BindlessResourceType::Count); ++i)
        {
            m_allocators[i].GarbageCollect();
        }
    }

    VkDescriptorSet BindlessDescriptorPool::GetNativeDescriptorSet()
    {
        return m_set;
    }

    uint32_t BindlessDescriptorPool::GetBindlessSrgBindingSlot()
    {
        return m_bindlessSrgDesc.m_bindlesSrgBindingSlot;
    }

    bool BindlessDescriptorPool::IsInitialized() const
    {
        return m_pool && m_pool->IsInitialized();
    }
} // namespace AZ::Vulkan

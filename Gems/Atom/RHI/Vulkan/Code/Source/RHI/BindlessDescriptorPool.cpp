/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BindlessDescriptorPool.h"

#include "BufferView.h"
#include "Device.h"
#include "ImageView.h"

#include <vulkan/vulkan.h>

namespace AZ::Vulkan
{
    // TODO(BINDLESS): This number is way too big and wastes memory. It's hardcoded here to match the unbounded array descriptor request
    // count in the pipeline layout.
    constexpr static uint32_t UnboundedArraySize = 100000u;

    void BindlessDescriptorPool::Init(Device& device)
    {
        m_device = &device;
        const uint32_t roTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture);
        const uint32_t rwTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture);
        const uint32_t roBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadBuffer);
        const uint32_t rwBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer);
        const uint32_t MaxBindlessIndices = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count);

        {
            DescriptorPool::Descriptor desc;
            desc.m_device = &device;
            
            desc.m_descriptorPoolSizes.resize_no_construct(MaxBindlessIndices);
            desc.m_descriptorPoolSizes[roTextureIndex] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, UnboundedArraySize };
            desc.m_descriptorPoolSizes[rwTextureIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UnboundedArraySize };
            desc.m_descriptorPoolSizes[roBufferIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UnboundedArraySize };
            desc.m_descriptorPoolSizes[rwBufferIndex] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UnboundedArraySize };
            desc.m_maxSets = 1;
            desc.m_collectLatency = 1;
            desc.m_updateAfterBind = true;

            m_pool = DescriptorPool::Create();
            m_pool->Init(desc);
        }

        {
            // TODO(BINDLESS): Here, we aren't using the SRG abstraction to create the native binding layout because we'd need a shader to
            // hook up reflection, similar to how the scene/view SRGs are reflected. For now, this layout is fixed but needs to match the
            // Bindless.azsli SRG declaration.
            VkDescriptorSetLayoutBinding bindings[MaxBindlessIndices];
      
            bindings[roTextureIndex].binding = roTextureIndex;
            bindings[roTextureIndex].descriptorCount = UnboundedArraySize;
            bindings[roTextureIndex].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindings[roTextureIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[rwTextureIndex].binding = rwTextureIndex;
            bindings[rwTextureIndex].descriptorCount = UnboundedArraySize;
            bindings[rwTextureIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            bindings[rwTextureIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[roBufferIndex].binding = roBufferIndex;
            bindings[roBufferIndex].descriptorCount = UnboundedArraySize;
            bindings[roBufferIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[roBufferIndex].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[rwBufferIndex].binding = rwBufferIndex;
            bindings[rwBufferIndex].descriptorCount = UnboundedArraySize;
            bindings[rwBufferIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[rwBufferIndex].stageFlags = VK_SHADER_STAGE_ALL;
            
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

            m_device->GetContext().CreateDescriptorSetLayout(m_device->GetNativeDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_pool->GetNativeDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_descriptorSetLayout;

            m_device->GetContext().AllocateDescriptorSets(m_device->GetNativeDevice(), &allocInfo, &m_set);
        }

        for (size_t i = 0; i != MaxBindlessIndices; ++i)
        {
            RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = UnboundedArraySize;
            desc.m_alignmentInBytes = 1;
            desc.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            desc.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocators[i].Init(desc);
        }
    }

    void BindlessDescriptorPool::Shutdown()
    {
        m_device->GetContext().FreeDescriptorSets(m_device->GetNativeDevice(), m_pool->GetNativeDescriptorPool(), 1, &m_set);
        m_device->GetContext().DestroyDescriptorSetLayout(m_device->GetNativeDevice(), m_descriptorSetLayout, nullptr);

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
        const uint32_t roTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture);
        
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        RHI::VirtualAddress address = m_allocators[roTextureIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);

        VkWriteDescriptorSet write = PrepareWrite(heapIndex, roTextureIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
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
        const uint32_t rwTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture);

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        RHI::VirtualAddress address = m_allocators[rwTextureIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);

        VkWriteDescriptorSet write = PrepareWrite(heapIndex, rwTextureIndex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
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
        const uint32_t roBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadBuffer);

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        RHI::VirtualAddress address = m_allocators[roBufferIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();
        VkWriteDescriptorSet write = PrepareWrite(heapIndex, roBufferIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
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
        const uint32_t rwBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer);

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        RHI::VirtualAddress address = m_allocators[rwBufferIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();
        VkWriteDescriptorSet write = PrepareWrite(heapIndex, rwBufferIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        m_device->GetContext().UpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);
        
        return heapIndex;
    }

    void BindlessDescriptorPool::DetachReadImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        const uint32_t roTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture);
        m_allocators[roTextureIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        const uint32_t rwTextureIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture);
        m_allocators[rwTextureIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        const uint32_t roBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadBuffer);
        m_allocators[roBufferIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        const uint32_t rwBufferIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer);
        m_allocators[rwBufferIndex].DeAllocate({ index });
    }

    void BindlessDescriptorPool::GarbageCollect()
    {
        for (size_t i = 0; i != static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count); ++i)
        {
            m_allocators[i].GarbageCollect();
        }
    }

    VkDescriptorSet BindlessDescriptorPool::GetNativeDescriptorSet()
    {
        return m_set;
    }
} // namespace AZ::Vulkan

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

        {
            DescriptorPool::Descriptor desc;
            desc.m_device = &device;

            desc.m_descriptorPoolSizes.resize_no_construct(4);
            desc.m_descriptorPoolSizes[0] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, UnboundedArraySize };
            desc.m_descriptorPoolSizes[1] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UnboundedArraySize };
            desc.m_descriptorPoolSizes[2] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UnboundedArraySize };
            desc.m_descriptorPoolSizes[3] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UnboundedArraySize };
            // desc.m_descriptorPoolSizes[4] = { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, UnboundedArraySize };
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
            VkDescriptorSetLayoutBinding bindings[4];

            bindings[0].binding = 0;
            bindings[0].descriptorCount = UnboundedArraySize;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[1].binding = 1;
            bindings[1].descriptorCount = UnboundedArraySize;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[2].binding = 2;
            bindings[2].descriptorCount = UnboundedArraySize;
            bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[2].stageFlags = VK_SHADER_STAGE_ALL;

            bindings[3].binding = 3;
            bindings[3].descriptorCount = UnboundedArraySize;
            bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[3].stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorBindingFlags bindingFlags[4];
            for (size_t i = 0; i != 4; ++i)
            {
                bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            }
            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingFlagsInfo.bindingCount = 4;
            bindingFlagsInfo.pBindingFlags = bindingFlags;

            // TODO(BINDLESS): The pipeline layout creation does not recognize an unbounded array of TLAS descriptors yet
            // We should add a pool of bindless TLAS descriptors as well

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = &bindingFlagsInfo;
            layoutInfo.bindingCount = 4;
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            layoutInfo.pBindings = bindings;

            vkCreateDescriptorSetLayout(m_device->GetNativeDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_pool->GetNativeDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_descriptorSetLayout;

            vkAllocateDescriptorSets(m_device->GetNativeDevice(), &allocInfo, &m_set);
        }

        for (size_t i = 0; i != 5; ++i)
        {
            RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = UnboundedArraySize;
            desc.m_alignmentInBytes = 1;
            desc.m_garbageCollectLatency = 3;
            desc.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocators[i].Init(desc);
        }
    }

    void BindlessDescriptorPool::Shutdown()
    {
        vkFreeDescriptorSets(m_device->GetNativeDevice(), m_pool->GetNativeDescriptorPool(), 1, &m_set);
        vkDestroyDescriptorSetLayout(m_device->GetNativeDevice(), m_descriptorSetLayout, nullptr);

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
        AZStd::array<RHI::ConstPtr<RHI::ImageView>, 1> span{ view };
        uint32_t index = static_cast<uint32_t>(m_allocators[0].Allocate(1, 1).m_ptr);

        VkWriteDescriptorSet write = PrepareWrite(index, 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = view->GetNativeImageView();
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return index;
    }

    uint32_t BindlessDescriptorPool::AttachReadWriteImage(ImageView* view)
    {
        AZStd::array<RHI::ConstPtr<RHI::ImageView>, 1> span{ view };
        uint32_t index = static_cast<uint32_t>(m_allocators[1].Allocate(1, 1).m_ptr);

        VkWriteDescriptorSet write = PrepareWrite(index, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = view->GetNativeImageView();
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return index;
    }

    uint32_t BindlessDescriptorPool::AttachReadBuffer(BufferView* view)
    {
        AZStd::array<const RHI::ConstPtr<RHI::BufferView>, 1> span{ view };
        uint32_t index = static_cast<uint32_t>(m_allocators[2].Allocate(1, 1).m_ptr);

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();

        VkWriteDescriptorSet write = PrepareWrite(index, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return index;
    }

    uint32_t BindlessDescriptorPool::AttachReadWriteBuffer(BufferView* view)
    {
        AZStd::array<RHI::ConstPtr<RHI::BufferView>, 1> span{ view };
        uint32_t index = static_cast<uint32_t>(m_allocators[3].Allocate(1, 1).m_ptr);

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();

        VkWriteDescriptorSet write = PrepareWrite(index, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return index;
    }

    uint32_t BindlessDescriptorPool::AttachTLAS(BufferView* view)
    {
        AZStd::array<RHI::ConstPtr<RHI::BufferView>, 1> span{ view };
        uint32_t index = static_cast<uint32_t>(m_allocators[4].Allocate(1, 1).m_ptr);

        const auto& viewDesc = view->GetDescriptor();
        const Vulkan::BufferMemoryView& bufferMemoryView = *static_cast<const Vulkan::Buffer&>(view->GetBuffer()).GetBufferMemoryView();

        VkWriteDescriptorSet write = PrepareWrite(index, 4, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

        VkWriteDescriptorSetAccelerationStructureKHR tlasInfo{};
        tlasInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        tlasInfo.accelerationStructureCount = 1;
        VkAccelerationStructureKHR tlas = view->GetNativeAccelerationStructure();
        tlasInfo.pAccelerationStructures = &tlas;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = bufferMemoryView.GetNativeBuffer();
        bufferInfo.offset = bufferMemoryView.GetOffset() + viewDesc.m_elementSize * viewDesc.m_elementOffset;
        bufferInfo.range = viewDesc.m_elementSize * viewDesc.m_elementCount;
        write.pBufferInfo = &bufferInfo;
        write.pNext = &tlasInfo;
        vkUpdateDescriptorSets(m_device->GetNativeDevice(), 1, &write, 0, nullptr);

        return index;
    }

    void BindlessDescriptorPool::DetachReadImage(uint32_t index)
    {
        m_allocators[0].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteImage(uint32_t index)
    {
        m_allocators[1].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadBuffer(uint32_t index)
    {
        m_allocators[2].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachReadWriteBuffer(uint32_t index)
    {
        m_allocators[3].DeAllocate({ index });
    }

    void BindlessDescriptorPool::DetachTLAS(uint32_t index)
    {
        m_allocators[4].DeAllocate({ index });
    }

    void BindlessDescriptorPool::GarbageCollect()
    {
        for (size_t i = 0; i != 5; ++i)
        {
            m_allocators[i].GarbageCollect();
        }
    }

    VkDescriptorSet BindlessDescriptorPool::GetNativeDescriptorSet()
    {
        return m_set;
    }
} // namespace AZ::Vulkan

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI/FreeListAllocator.h>

namespace AZ::Vulkan
{
    class Device;
    class BindlessDescriptorPool
    {
    public:
        void Init(Device& device);
        void Shutdown();

        uint32_t AttachReadBuffer(BufferView* view);
        uint32_t AttachReadWriteBuffer(BufferView* view);
        uint32_t AttachReadImage(ImageView* view);
        uint32_t AttachReadWriteImage(ImageView* view);
        uint32_t AttachTLAS(BufferView* view);

        void DetachReadBuffer(uint32_t index);
        void DetachReadWriteBuffer(uint32_t index);
        void DetachReadImage(uint32_t index);
        void DetachReadWriteImage(uint32_t index);
        void DetachTLAS(uint32_t index);

        void GarbageCollect();

        VkDescriptorSet GetNativeDescriptorSet();

    private:
        VkWriteDescriptorSet PrepareWrite(uint32_t index, uint32_t binding, VkDescriptorType type);

        Device* m_device = nullptr;
        RHI::Ptr<DescriptorPool> m_pool;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_set;

        RHI::FreeListAllocator m_allocators[5];
    };
}

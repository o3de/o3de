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
#include <Atom/RHI/DeviceShaderResourceGroupData.h>

namespace AZ::Vulkan
{
    class Device;

    //! This class is responsible for managing the global bindless descriptor heap. It provides
    //! support for this heap via both unbounded arrays.
    class BindlessDescriptorPool
    {
    public:
        RHI::ResultCode Init(Device& device, const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc);
        void Shutdown();

        //! Add/Update a read only buffer descriptor to the global bindless heap 
        uint32_t AttachReadBuffer(BufferView* view);

        //! Add/Update a read write buffer descriptor to the global bindless heap 
        uint32_t AttachReadWriteBuffer(BufferView* view);

        //! Add/Update a read only image descriptor to the global bindless heap 
        uint32_t AttachReadImage(ImageView* view);

        //! Add/Update a read write image descriptor to the global bindless heap 
        uint32_t AttachReadWriteImage(ImageView* view);

        //! Add/Update a read only cubemap image descriptor to the global bindless heap 
        uint32_t AttachReadCubeMapImage(ImageView* view);

        //! Remove the index associated with a read only buffer descriptor from the free list allocator
        void DetachReadBuffer(uint32_t index);

        //! Remove the index associated with a read write buffer descriptor from the free list allocator
        void DetachReadWriteBuffer(uint32_t index);

        //! Remove the index associated with a read only image descriptor from the free list allocator
        void DetachReadImage(uint32_t index);

        //! Remove the index associated with a read write image descriptor from the free list allocator
        void DetachReadWriteImage(uint32_t index);

        //! Remove the index associated with a read only cubemapimage descriptor from the free list allocator
        void DetachReadCubeMapImage(uint32_t index);

        //! Garbage collect all the free list allocators related to all the bindless resource types
        void GarbageCollect();

        //! Return the descriptor set related to the global bindless descriptor set
        VkDescriptorSet GetNativeDescriptorSet();

        //! Return the binding slot for the bindless srg
        uint32_t GetBindlessSrgBindingSlot();

        //! Boolean to return true if the pool is initialized
        bool IsInitialized() const;

    private:
        VkWriteDescriptorSet PrepareWrite(uint32_t index, uint32_t binding, VkDescriptorType type);

        Device* m_device = nullptr;
        RHI::Ptr<DescriptorPool> m_pool;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_set;

        RHI::FreeListAllocator m_allocators[static_cast<uint32_t>(AZ::RHI::BindlessResourceType::Count)];

        // Descriptor to hold information related to binding indices of bindless srg
        AZ::RHI::BindlessSrgDescriptor m_bindlessSrgDesc;

        // Mutex to protect bindless heap related updates
        AZStd::mutex m_mutex;
    };
}

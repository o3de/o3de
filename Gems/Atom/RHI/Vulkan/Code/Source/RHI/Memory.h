/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        /**
         * It holds Vulkan's native VkDeviceMemory.
         * When it is removed, VkDeviceMemory will be freed.
         * Allows simultaneous mappings of non overlapping ranges. 
        */ 
        class Memory
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(Memory, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Memory, "A5D92BC7-2BD8-46BB-85AB-ED549982130E", Base);

            struct Descriptor
            {
                VkDeviceSize m_sizeInBytes = 0;
                uint32_t m_memoryTypeIndex = 0;
                RHI::BufferBindFlags m_bufferBindFlags = RHI::BufferBindFlags::None;
            };

            ~Memory() = default;

            static RHI::Ptr<Memory> Create();
            RHI::ResultCode Init(Device& device, const Descriptor& descriptor);

            const VkDeviceMemory GetNativeDeviceMemory();
            const Descriptor& GetDescriptor() const;

            // It maps a memory and returns its mapped address.
            CpuVirtualAddress Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess);

            // Unmap have to be called after Map is called for memory.
            void Unmap(size_t offset, RHI::HostMemoryAccess hostAccess);
            
        private:
            Memory() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            Descriptor m_descriptor;
            VkDeviceMemory m_nativeDeviceMemory = VK_NULL_HANDLE;
            CpuVirtualAddress m_mappedMemory = nullptr;
            VkMemoryPropertyFlags m_memoryPropertyFlags = 0;

            using MappingMap = AZStd::map<size_t, size_t>;
            MappingMap m_mappings;

            AZStd::mutex m_mutex;
        };
    }
}

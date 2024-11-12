/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Vulkan
    {
        //! A BufferMemory represents a Vulkan's Buffer that will be sub allocated into one
        //! or multiple RHI Buffer Resources. Each of the RHI Buffers reference a section of the Vulkan Buffer.
        //! The Vulkan Buffer sits on top of a Vulkan Memory Region.
        //! When it is removed, the native buffer will be freed.
        //!      _______________________________________________________________
        //!     |                           VmaAllocation                       |
        //!     |_______________________________________________________________|
        //!      _______________________________  ______________________________
        //!     |  VkBuffer (BufferMemoryView)  ||  VkBuffer (BufferMemoryView) |
        //!     |_______________________________||______________________________|
        //!      _______________  ______________  ______________________________
        //!     |   RHI Buffer  ||  RHI Buffer  ||         RHI Buffer           |
        //!     |_______________||______________||______________________________|
        //!
        class BufferMemory
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(BufferMemory, AZ::ThreadPoolAllocator);
            AZ_RTTI(BufferMemory, "{39053FBD-CE0E-44E8-A9BF-29C4014C3958}", Base);

             struct Descriptor : public RHI::BufferDescriptor
             {
                 Descriptor() = default;
                 Descriptor(const RHI::BufferDescriptor& desc, RHI::HeapMemoryLevel memoryLevel);

                 RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
             };

            ~BufferMemory() = default;

            static RHI::Ptr<BufferMemory> Create();
            RHI::ResultCode Init(Device& device, const MemoryView& memoryView, const Descriptor& descriptor);
            RHI::ResultCode Init(Device& device, const Descriptor& descriptor);

            // It maps a memory and returns its mapped address.
            CpuVirtualAddress Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess);

            // Unmap have to be called after Map is called for memory.
            void Unmap(size_t offset, RHI::HostMemoryAccess hostAccess);

            const VkBuffer GetNativeBuffer();
            const Descriptor& GetDescriptor() const;
            VkSharingMode GetSharingMode() const;

            VkDeviceMemory GetNativeDeviceMemory() const;
            size_t GetMemoryViewOffset() const;
            size_t GetAllocationSize() const;
            size_t GetSize() const;
            const MemoryView& GetMemoryView() const;

        private:
            BufferMemory() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            Descriptor m_descriptor;
            VkBuffer m_vkBuffer = VK_NULL_HANDLE;
            MemoryView m_memoryView;
            VkSharingMode m_sharingMode = VK_SHARING_MODE_MAX_ENUM;
        };
    }
}

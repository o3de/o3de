/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <RHI/Vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        //! Represents a VMA memory allocation.
        //! The allocation may be part of a larger memory block (sub allocated) or be a dedicated memory block.
        class VulkanMemoryAllocation :
            public RHI::DeviceObject 
        {
        public:
            //! Creates a memory allocation object
            static RHI::Ptr<VulkanMemoryAllocation> Create();

            //! Initialize a memmory allocation from a VMA allocation.
            void Init(Device& device, const VmaAllocation& alloc);

            //! Returns the offset relative to the base memory address in bytes.
            size_t GetOffset() const;

            //! Returns the size of the memory region in bytes.
            size_t GetSize() const;

            //! Returns the size of the memory block in bytes.
            size_t GetBlockSize() const;

            //! A convenience method to map the resource region spanned by the view for CPU access.
            CpuVirtualAddress Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess);

            //! A convenience method for unmapping the resource region spanned by the view.
            void Unmap(size_t offset, RHI::HostMemoryAccess hostAccess);

            //! Returns the VMA allocation
            VmaAllocation GetVmaAllocation() const;

            //! Returns the vulkan native memory used by the allocation.
            VkDeviceMemory GetNativeDeviceMemory() const;

        private:
            VulkanMemoryAllocation() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            VmaAllocationInfo GetAllocationInfo() const;

            VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
            size_t m_size = 0;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/BufferMemory.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<BufferMemory> BufferMemory::Create()
        {
            return aznew BufferMemory();
        }

// @CYA EDIT: Replace O3DE allocator by VMA
        RHI::ResultCode BufferMemory::Init(Device& device, const Descriptor& descriptor)
        {
            Base::Init(device);
            m_descriptor = descriptor;

            AZ_Assert(descriptor.m_sharedQueueMask != RHI::HardwareQueueClassMask::None, "Invalid shared queue mask");
            AZStd::vector<uint32_t> queueFamilies(device.GetCommandQueueContext().GetQueueFamilyIndices(descriptor.m_sharedQueueMask));

            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.size = descriptor.m_byteCount;
            createInfo.usage = device.GetBufferUsageFlagBitsUnderRestrictions(descriptor.m_bindFlags);
            // Trying to guess here if the buffers are going to be used as attachments. Maybe it would be better to add an explicit flag in
            // the descriptor.
            createInfo.sharingMode =
                (RHI::CheckBitsAny(
                     descriptor.m_bindFlags,
                     RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::Predication | RHI::BufferBindFlags::Indirect) ||
                 (queueFamilies.size()) <= 1)
                ? VK_SHARING_MODE_EXCLUSIVE
                : VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size());
            createInfo.pQueueFamilyIndices = queueFamilies.empty() ? nullptr : queueFamilies.data();

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            VmaAllocator allocator = device.GetMemoryAllocator();
            if (vmaCreateBuffer(allocator, &createInfo, &allocInfo, &m_vkBuffer, &m_vmaAllocation, &m_vmaAllocationInfo) != VK_SUCCESS)
                return RHI::ResultCode::OutOfMemory;

            return RHI::ResultCode::Success;
        }
// @CYA END

        RHI::ResultCode BufferMemory::Init(Device& device, const MemoryView& memoryView, const Descriptor& descriptor)
        {
            return Init(device, device.CreateBufferResouce(descriptor), memoryView, descriptor);
        }

        RHI::ResultCode BufferMemory::Init(Device& device, VkBuffer vkBuffer, const MemoryView& memoryView, const Descriptor& descriptor)
        {
            AZ_Assert(vkBuffer != VK_NULL_HANDLE, "Null vulkan buffer");
            Base::Init(device);
            m_memoryView = memoryView;
            m_vkBuffer = vkBuffer;
            m_descriptor = descriptor;

            VkResult vkResult = device.GetContext().BindBufferMemory(
                device.GetNativeDevice(), m_vkBuffer, memoryView.GetMemory()->GetNativeDeviceMemory(), memoryView.GetOffset());
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            return result;
        }

// @CYA EDIT: Replace O3DE allocator by VMA
        bool BufferMemory::IsVmaAllocated() const
        {
            return m_vmaAllocation != VK_NULL_HANDLE;
        }

        const VmaAllocationInfo& BufferMemory::GetVmaAllocationInfo() const
        {
            AZ_Assert(IsVmaAllocated(), "Buffer memory was not allocated with VMA");
            return m_vmaAllocationInfo;
        }

        CpuVirtualAddress BufferMemory::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            Device& device = static_cast<Device&>(GetDevice());

            void* mappedPtr;
            VkResult vkResult = vmaMapMemory(device.GetMemoryAllocator(), m_vmaAllocation, &mappedPtr);
            if (vkResult != VK_SUCCESS)
                return nullptr;

            if (hostAccess == RHI::HostMemoryAccess::Read)
            {
                // VMA will check if this is necessary
                vkResult = vmaInvalidateAllocation(device.GetMemoryAllocator(), m_vmaAllocation, offset, size);
                if (vkResult != VK_SUCCESS)
                    return nullptr;
            }

            return static_cast<CpuVirtualAddress>(mappedPtr);
        }

        void BufferMemory::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            Device& device = static_cast<Device&>(GetDevice());

            if (hostAccess == RHI::HostMemoryAccess::Write)
                vmaFlushAllocation(device.GetMemoryAllocator(), m_vmaAllocation, offset, VK_WHOLE_SIZE);

            vmaUnmapMemory(device.GetMemoryAllocator(), m_vmaAllocation);
        }
// @CYA END

        const VkBuffer BufferMemory::GetNativeBuffer()
        {
            return m_vkBuffer;
        }

        const BufferMemory::Descriptor& BufferMemory::GetDescriptor() const
        {
            return m_descriptor;
        }

        void BufferMemory::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_vkBuffer), name.data(), VK_OBJECT_TYPE_BUFFER, static_cast<Device&>(GetDevice()));
            }

            if (m_memoryView.GetAllocationType() == MemoryAllocationType::Unique)
            {
                m_memoryView.SetName(name);
            }
        }

        void BufferMemory::Shutdown()
        {
            if (m_vkBuffer != VK_NULL_HANDLE)
            {
                Device& device = static_cast<Device&>(GetDevice());
// @CYA EDIT: Replace O3DE allocator by VMA
                vmaDestroyBuffer(device.GetMemoryAllocator(), m_vkBuffer, m_vmaAllocation);
                m_vkBuffer = VK_NULL_HANDLE;
                m_vmaAllocation = VK_NULL_HANDLE;
// @CYA END
            }

            m_memoryView = MemoryView();
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom_RHI_Vulkan_Platform.h>
#include <RHI/BufferMemory.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<BufferMemory> BufferMemory::Create()
        {
            return aznew BufferMemory();
        }

        RHI::ResultCode BufferMemory::Init(Device& device, const MemoryView& memoryView, const Descriptor& descriptor)
        {
            BufferCreateInfo createInfo = device.BuildBufferCreateInfo(descriptor);
            // Create the buffer using a specific memory location. This function doesn't allocate new memory, it binds
            // the provided memory (with the proper offset) to the buffer.
            VkResult vkResult = vmaCreateAliasingBuffer2(
                device.GetVmaAllocator(),
                memoryView.GetAllocation()->GetVmaAllocation(),
                memoryView.GetOffset(),
                &createInfo.m_vkCreateInfo,
                &m_vkBuffer);

            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            Base::Init(device);
            m_memoryView = memoryView;
            m_descriptor = descriptor;

            return result;
        }

        RHI::ResultCode BufferMemory::Init(Device& device, const Descriptor& descriptor)
        {
            Base::Init(device);
            m_descriptor = descriptor;
            BufferCreateInfo createInfo = device.BuildBufferCreateInfo(descriptor);
            VmaAllocationCreateInfo allocInfo = GetVmaAllocationCreateInfo(descriptor.m_heapMemoryLevel);
            VmaAllocation vmaAlloc;
            VkResult vkResult;
            // Creates the buffer, allocates new memory and bind it to the buffer.
            vkResult = vmaCreateBufferWithAlignment(
                device.GetVmaAllocator(),
                &createInfo.m_vkCreateInfo,
                &allocInfo,
                RHI::IsPowerOfTwo(descriptor.m_alignment) ? descriptor.m_alignment : 1,
                &m_vkBuffer,
                &vmaAlloc,
                nullptr);
            
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            RHI::Ptr<VulkanMemoryAllocation> alloc = VulkanMemoryAllocation::Create();
            alloc->Init(device, vmaAlloc);
            m_memoryView = MemoryView(alloc);
            return RHI::ResultCode::Success;
        }

        CpuVirtualAddress BufferMemory::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetAllocation()->Map(m_memoryView.GetOffset() + offset, size, hostAccess);
        }

        void BufferMemory::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetAllocation()->Unmap(m_memoryView.GetOffset() + offset, hostAccess);
        }

        const VkBuffer BufferMemory::GetNativeBuffer()
        {
            return m_vkBuffer;
        }

        const BufferMemory::Descriptor& BufferMemory::GetDescriptor() const
        {
            return m_descriptor;
        }

        size_t BufferMemory::GetSize() const
        {
            return m_memoryView.GetSize();
        }

        void BufferMemory::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_vkBuffer), name.data(), VK_OBJECT_TYPE_BUFFER, static_cast<Device&>(GetDevice()));
            }

             m_memoryView.SetName(name);
        }

        void BufferMemory::Shutdown()
        {
            if (m_vkBuffer != VK_NULL_HANDLE)
            {
                Device& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyBuffer(device.GetNativeDevice(), m_vkBuffer, VkSystemAllocator::Get());
                m_vkBuffer = VK_NULL_HANDLE;
            }

            m_memoryView = MemoryView();
        }

        BufferMemory::Descriptor::Descriptor(const RHI::BufferDescriptor& desc, RHI::HeapMemoryLevel memoryLevel)
            : RHI::BufferDescriptor(desc)
            , m_heapMemoryLevel(memoryLevel)
        {
        }
    } // namespace Vulkan
}

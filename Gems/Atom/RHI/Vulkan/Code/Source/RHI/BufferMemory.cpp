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

        CpuVirtualAddress BufferMemory::Map(size_t offset, size_t size, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetMemory()->Map(m_memoryView.GetOffset() + offset, size, hostAccess);
        }

        void BufferMemory::Unmap(size_t offset, RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryView.GetMemory()->Unmap(m_memoryView.GetOffset() + offset, hostAccess);
        }

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
                device.DestroyBufferResource(m_vkBuffer);
                m_vkBuffer = VK_NULL_HANDLE;
            }

            m_memoryView = MemoryView();
        }
    }
}

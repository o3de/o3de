/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandPool.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<CommandPool> CommandPool::Create()
        {
            return aznew CommandPool();
        }

        RHI::ResultCode CommandPool::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null");
            Base::Init(*descriptor.m_device);
            m_descriptor = descriptor;

            const RHI::ResultCode result = BuildNativeCommandPool();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        void CommandPool::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeCommandPool), name.data(), VK_OBJECT_TYPE_COMMAND_POOL, static_cast<Device&>(GetDevice()));
            }
        }

        void CommandPool::Shutdown()
        {
            m_freeCommandLists.clear();
            m_commandLists.clear();
            if (m_nativeCommandPool != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyCommandPool(device.GetNativeDevice(), m_nativeCommandPool, VkSystemAllocator::Get());
                m_nativeCommandPool = VK_NULL_HANDLE;
            }
            Base::Shutdown();
        }

        RHI::ResultCode CommandPool::BuildNativeCommandPool()
        {
            auto& device = static_cast<Device&>(GetDevice());
            
            VkCommandPoolCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.queueFamilyIndex = m_descriptor.m_queueFamilyIndex;

            const VkResult result = device.GetContext().CreateCommandPool(
                device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeCommandPool);
            AssertSuccess(result);

            return ConvertResult(result);
        }

        VkCommandPool CommandPool::GetNativeCommandPool() const
        {
            return m_nativeCommandPool;
        }

        RHI::Ptr<CommandList> CommandPool::AllocateCommandList(VkCommandBufferLevel level)
        {
            RHI::Ptr<CommandList> cmdList;
            // Search the free list first
            auto it = AZStd::find_if(m_freeCommandLists.begin(), m_freeCommandLists.end(), [&level](const RHI::Ptr<CommandList>& cmdList) 
            { 
                return cmdList->m_descriptor.m_level == level;
            });

            if (it != m_freeCommandLists.end())
            {
                cmdList = *it;
                m_commandLists.push_back(cmdList);
                m_freeCommandLists.erase(it);
                return cmdList;
            }

            // Create a new CommandList
            auto& device = static_cast<Device&>(GetDevice());
            cmdList = CommandList::Create();
            CommandList::Descriptor cmdListDesc;
            cmdListDesc.m_device = &device;
            cmdListDesc.m_commandPool = this;
            cmdListDesc.m_level = level;
            if (cmdList->Init(cmdListDesc) != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to allocate command list");
                return RHI::Ptr<CommandList>();
            }

            m_commandLists.push_back(cmdList);
            return cmdList;
        }

        const CommandPool::Descriptor& CommandPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        void CommandPool::Reset()
        {
            auto& device = static_cast<Device&>(GetDevice());
            for (RHI::Ptr<CommandList>& cmdList : m_commandLists)
            {
                cmdList->Reset();
            }
            m_freeCommandLists.insert(m_freeCommandLists.end(), AZStd::make_move_iterator(m_commandLists.begin()), AZStd::make_move_iterator(m_commandLists.end()));
            m_commandLists.clear();
            AssertSuccess(device.GetContext().ResetCommandPool(device.GetNativeDevice(), m_nativeCommandPool, 0));
        }        
    }
}

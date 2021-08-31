/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandListAllocator.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        namespace Internal
        {
            ///////////////////////////////////////////////////////////////////
            // CommandPoolAllocator
            ///////////////////////////////////////////////////////////////////
            void CommandPoolFactory::Init(const Descriptor& descriptor)
            {
                m_descriptor = descriptor;
            }

            RHI::Ptr<CommandPool> CommandPoolFactory::CreateObject()
            {
                RHI::Ptr<CommandPool> commandPool = CommandPool::Create();
                if (commandPool->Init(m_descriptor) != RHI::ResultCode::Success)
                {
                    AZ_Printf("Vulkan", "Failed to initialize CommandPool");
                    return RHI::Ptr<CommandPool>();
                }

                return commandPool;
            }

            void CommandPoolFactory::ResetObject(CommandPool& commandPool)
            {
                commandPool.Reset();
            }

            void CommandPoolFactory::ShutdownObject(CommandPool& commandPool, bool isPoolShutdown)
            {
                AZ_UNUSED(isPoolShutdown);
                commandPool.Shutdown();
            }

            bool CommandPoolFactory::CollectObject(CommandPool& commandPool)
            {
                AZ_UNUSED(commandPool);
                return true;
            }

            const CommandPoolFactory::Descriptor& CommandPoolFactory::GetDescriptor() const
            {
                return m_descriptor;
            }

            ///////////////////////////////////////////////////////////////////
            // CommandListSubAllocator
            ///////////////////////////////////////////////////////////////////
            void CommandListSubAllocator::Init(CommandPoolAllocator& commandPoolAllocator)
            {
                m_commandPoolAllocator = &commandPoolAllocator;
            }

            RHI::Ptr<CommandList> CommandListSubAllocator::Allocate(VkCommandBufferLevel level)
            {
                if (!m_commandPool)
                {
                    m_commandPool = m_commandPoolAllocator->Allocate();
                }

                return m_commandPool->AllocateCommandList(level);
            }

            void CommandListSubAllocator::Reset()
            {
                if (m_commandPool)
                {
                    m_commandPoolAllocator->DeAllocate(m_commandPool);
                    m_commandPool = nullptr;
                }
            }
        }

        RHI::ResultCode CommandListAllocator::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(m_isInitialized == false, "CommandListAllocator already initialized!");
            AZ_Assert(descriptor.m_frameCountMax <= MaxFamilyQueueCount, "Too many family types");

            for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < m_descriptor.m_familyQueueCount; ++queueFamilyIndex)
            {
                Internal::CommandPoolAllocator& commadPoolAllocator = m_commandPoolAllocators[queueFamilyIndex];

                Internal::CommandPoolAllocator::Descriptor commandPoolAllocatorDescriptor;
                commandPoolAllocatorDescriptor.m_device = m_descriptor.m_device;
                commandPoolAllocatorDescriptor.m_queueFamilyIndex = queueFamilyIndex;
                commandPoolAllocatorDescriptor.m_collectLatency = descriptor.m_frameCountMax;
                commadPoolAllocator.Init(commandPoolAllocatorDescriptor);

                m_commandListSubAllocators[queueFamilyIndex].SetInitFunction([&commadPoolAllocator]
                (Internal::CommandListSubAllocator& subAllocator)
                {
                    subAllocator.Init(commadPoolAllocator);
                });
            }
            
            m_isInitialized = true;
            return RHI::ResultCode::Success;
        }

        RHI::Ptr<CommandList> CommandListAllocator::Allocate(uint32_t familyQueueIndex, VkCommandBufferLevel level)
        {
            return m_commandListSubAllocators[familyQueueIndex].GetStorage().Allocate(level);
        }

        void CommandListAllocator::Collect()
        {
            for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
            {
                m_commandListSubAllocators[queueIdx].ForEach([](Internal::CommandListSubAllocator& commandListSubAllocator)
                {
                    commandListSubAllocator.Reset();
                });

                m_commandPoolAllocators[queueIdx].Collect();
            }
        }

        void CommandListAllocator::Shutdown()
        {
            if (m_isInitialized)
            {
                for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
                {
                    m_commandListSubAllocators[queueIdx].ForEach([](Internal::CommandListSubAllocator& commandListSubAllocator)
                    {
                        commandListSubAllocator.Reset();
                    });

                    m_commandListSubAllocators[queueIdx].Clear();
                    m_commandPoolAllocators[queueIdx].Shutdown();
                }
                m_isInitialized = false;
            }
        }
    }
}

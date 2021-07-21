/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Metal_precompiled.h"

#include <RHI/CommandList.h>
#include <RHI/CommandListPool.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Metal
    {
        void CommandListFactory::ResetObject(CommandList& commandList)
        {
            commandList.Reset();
        }

        RHI::Ptr<CommandList> CommandListFactory::CreateObject()
        {
            RHI::Ptr<CommandList> commandList = CommandList::Create();
            commandList->Init(m_descriptor.m_hardwareQueueClass, m_descriptor.m_device);
            return AZStd::move(commandList);
        }

        void CommandListFactory::ShutdownObject(CommandList& commandList, bool isPoolShutdown)
        {
            commandList.Shutdown();
            Base::ShutdownObject(commandList, isPoolShutdown);
        }

        void CommandListFactory::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        CommandListSubAllocator::~CommandListSubAllocator()
        {
            Reset();
        }
        
        void CommandListSubAllocator::Init(CommandListPool& commandListPool)
        {
            m_commandListPool = &commandListPool;
        }
        
        CommandList* CommandListSubAllocator::Allocate()
        {
            CommandList* commandList = m_commandListPool->Allocate();
            m_activeLists.push_back(commandList);
            return commandList;
        }
        
        void CommandListSubAllocator::Reset()
        {
            m_commandListPool->DeAllocate(m_activeLists.data(), static_cast<uint32_t>(m_activeLists.size()));
            m_activeLists.clear();
        }

        void CommandListAllocator::Init(const Descriptor& descriptor, Device* device)
        {
            for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
            {
                CommandListPool& commandListPool = m_commandListPools[queueIdx];
                
                CommandListPool::Descriptor commandListPoolDescriptor;
                commandListPoolDescriptor.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(queueIdx);
                commandListPoolDescriptor.m_device = device;
                commandListPoolDescriptor.m_collectLatency = descriptor.m_frameCountMax;
                commandListPool.Init(commandListPoolDescriptor);

                m_commandListSubAllocators[queueIdx].SetInitFunction([this, &commandListPool]
                                                                     (CommandListSubAllocator& subAllocator)
                                                                     {
                                                                         subAllocator.Init(commandListPool);
                                                                     });
            }
            
            m_isInitialized = true;
        }

        void CommandListAllocator::Shutdown()
        {
            if (m_isInitialized)
            {
                for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
                {
                    m_commandListSubAllocators[queueIdx].Clear();
                    m_commandListPools[queueIdx].Shutdown();
                }
                m_isInitialized = false;
            }
        }
        
        CommandList* CommandListAllocator::Allocate(RHI::HardwareQueueClass hardwareQueueClass)
        {
            AZ_Assert(m_isInitialized, "CommandListAllocator is not initialized!");
            return m_commandListSubAllocators[static_cast<uint32_t>(hardwareQueueClass)].GetStorage().Allocate();
        }
        
        void CommandListAllocator::Collect()
        {
            for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
            {
                m_commandListSubAllocators[queueIdx].ForEach([](CommandListSubAllocator& commandListSubAllocator)
                {
                    commandListSubAllocator.Reset();
                });
                m_commandListPools[queueIdx].Collect();
            }
        }
    }
}

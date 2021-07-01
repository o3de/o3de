/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class CommandList;

        class CommandListFactory
            : public RHI::ObjectFactoryBase<CommandList>
        {
            using Base = RHI::ObjectFactoryBase<CommandList>;
        public:
            struct Descriptor
            {
                RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
                Device* m_device = nullptr;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<CommandList> CreateObject();

            void ResetObject(CommandList& commandList);
            void ShutdownObject(CommandList& commandList, bool isPoolShutdown);

        private:
            Descriptor m_descriptor;
        };

        struct CommandListPoolTraits : public RHI::ObjectPoolTraits
        {
            using ObjectType = CommandList;
            using ObjectFactoryType = CommandListFactory;
            using MutexType = AZStd::recursive_mutex;
        };

        using CommandListPool = RHI::ObjectPool<CommandListPoolTraits>;

        
        ///////////////////////////////////////////////////////////////////
        // CommandListSubAllocator
        class CommandListSubAllocator final
        {
        public:
            ~CommandListSubAllocator();
            
            void Init(CommandListPool& commandListPool);
            CommandList* Allocate();
            
            void Reset();
            
        private:
            CommandListPool* m_commandListPool = nullptr;
            AZStd::vector<CommandList*> m_activeLists;
        };

        class CommandListAllocator final
        {
        public:
            CommandListAllocator() = default;
            
            struct Descriptor
            {
                // The maximum number of frames to keep buffered on the CPU timeline.
                uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;            
            };
            void Init(const Descriptor& descriptor, Device* device);
            
            void Shutdown();
            
            CommandList* Allocate(RHI::HardwareQueueClass hardwareQueueClass);

            // Call this once per frame to retire the current frame and reclaim
            // elements from completed frames
            void Collect();
            
        private:
            CommandListPool* m_commandListPool = nullptr;
            AZStd::vector<CommandList*> m_activeLists;
            
            AZStd::array<CommandListPool, RHI::HardwareQueueClassCount> m_commandListPools;
            AZStd::array<RHI::ThreadLocalContext<CommandListSubAllocator>, RHI::HardwareQueueClassCount> m_commandListSubAllocators;

            bool m_isInitialized = false;
        };
    }
}

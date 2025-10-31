/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/unordered_map.h>
#include <RHI/CommandPool.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        namespace Internal
        {
            class CommandPoolFactory final
                : public RHI::ObjectFactoryBase<CommandPool>
            {
                using Base = RHI::ObjectFactoryBase<CommandPool>;
            public:
                using Descriptor = CommandPool::Descriptor;

                void Init(const Descriptor& descriptor);

                RHI::Ptr<CommandPool> CreateObject();

                void ResetObject(CommandPool& commandPool);
                void ShutdownObject(CommandPool& commandPool, bool isPoolShutdown);
                bool CollectObject(CommandPool& commandPool);

                const Descriptor& GetDescriptor() const;

            private:
                Descriptor m_descriptor;
            };

            struct CommandPoolAllocatorTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = CommandPool;
                using ObjectFactoryType = CommandPoolFactory;
                using MutexType = AZStd::recursive_mutex;
            };

            using CommandPoolAllocator = RHI::ObjectPool<CommandPoolAllocatorTraits>;

            class CommandListSubAllocator final
            {
            public:
                CommandListSubAllocator() = default;
                CommandListSubAllocator(const CommandListSubAllocator&) = delete;

                void Init(CommandPoolAllocator& commandPoolAllocator);

                RHI::Ptr<CommandList> Allocate(VkCommandBufferLevel level);

                void Reset();

            private:
                CommandPoolAllocator* m_commandPoolAllocator = nullptr;
                CommandPool* m_commandPool = nullptr;
            };
        }

        /**
        * CommandListAllocator combines the CommandListPool, CommandAllocatorPool,
        * and CommandListSubAllocator utilities into a complete allocator implementation
        * that load balances across threads with almost zero contention.
        *
        * This class is best used with a job system, with 1 job per command list. The job
        * should close the command list on completion, because the next command list
        * recording job on the same thread will use the same internal linear allocator
        * (command allocator).
        *
        * Each Allocate call pulls from the thread-local command list sub-allocator.
        * CommandPools are reseted as a whole when they are collected and all command lists
        * from the pool are recycled.
        */
        class CommandListAllocator final
            : public RHI::Object
        {
            using Base = RHI::Object;

        public:
            AZ_CLASS_ALLOCATOR(CommandListAllocator, AZ::SystemAllocator);

            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
                uint32_t m_familyQueueCount = 0;
            };

            CommandListAllocator() = default;
            ~CommandListAllocator() = default;

            RHI::ResultCode Init(const Descriptor& descriptor);
            RHI::Ptr<CommandList> Allocate(uint32_t familyQueueIndex, VkCommandBufferLevel level);
            void Collect();
            void Shutdown();

        private:

            Descriptor m_descriptor;
            static const uint32_t MaxFamilyQueueCount = 10;
            AZStd::array<Internal::CommandPoolAllocator, MaxFamilyQueueCount> m_commandPoolAllocators;
            AZStd::array<RHI::ThreadLocalContext<Internal::CommandListSubAllocator>, MaxFamilyQueueCount> m_commandListSubAllocators;
            bool m_isInitialized = false;
        };
    }
}

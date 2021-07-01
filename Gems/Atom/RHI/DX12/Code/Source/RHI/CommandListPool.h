/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace DX12
    {
        class CommandList;
        class DescriptorContext;
        class Device;

        namespace Internal
        {
            ///////////////////////////////////////////////////////////////////////
            // CommandAllocatorPool

            class CommandAllocatorFactory final
                : public RHI::ObjectFactoryBase<ID3D12CommandAllocator>
            {
            public:
                struct Descriptor
                {
                    RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;

                    RHI::Ptr<ID3D12DeviceX> m_dx12Device;
                };

                void Init(const Descriptor& descriptor);

                RHI::Ptr<ID3D12CommandAllocator> CreateObject();

                void ResetObject(ID3D12CommandAllocator& allocator);

            private:
                Descriptor m_descriptor;
            };

            struct CommandAllocatorPoolTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = ID3D12CommandAllocator;
                using ObjectFactoryType = CommandAllocatorFactory;
                using MutexType = AZStd::mutex;
            };

            /**
             * In DirectX 12, Command Allocators are linear memory allocators for command list.
             * The recommended practice by nVidia / AMD is to keep N * T of them around, where
             * N is the number of buffered frames, T is the number of threads. The CommandAllocatorPool
             * handles allocating and retiring command allocators in a round-robin fashion.
             * The class used by CommandListAllocator and CommandListSubAllocator.
             */
            using CommandAllocatorPool = RHI::ObjectPool<CommandAllocatorPoolTraits>;

            ///////////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////////
            // CommandListPool

            class CommandListFactory final
                : public RHI::ObjectFactoryBase<CommandList>
            {
                using Base = RHI::ObjectFactoryBase<CommandList>;
            public:
                struct Descriptor
                {
                    Device* m_device = nullptr;

                    RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;

                    AZStd::shared_ptr<DescriptorContext> m_descriptorContext;
                };

                void Init(const Descriptor& descriptor);

                RHI::Ptr<CommandList> CreateObject(ID3D12CommandAllocator* commandAllocator);

                void ResetObject(CommandList& commandList, ID3D12CommandAllocator* commandAllocator);
                void ShutdownObject(CommandList& commandList, bool isPoolShutdown);
                bool CollectObject(CommandList& commandList);

            private:
                Descriptor m_descriptor;
            };

            struct CommandListPoolTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = CommandList;
                using ObjectFactoryType = CommandListFactory;
                using MutexType = AZStd::recursive_mutex;
            };

            /**
             * CommandListPool is a simple round-robin allocator of command lists. It takes
             * a lock with each allocation, and requires the CommandAllocator instance associate
             * with the new command list instance. CommandAllocators are not thread-safe and are
             * pooled separately from command list. The CommandListAllocator class combines
             * the CommandListPool and CommandAllocatorPools together with a per-thread sub-allocator
             * to facilitate more ideal allocation of command lists.
             */
            using CommandListPool = RHI::ObjectPool<CommandListPoolTraits>;

            ///////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            // CommandListSubAllocator

            /**
             * CommandListSubAllocator is intended for use across a single thread. It grabs
             * A single command allocator for the entire frame, and each command list allocated
             * uses that command allocator. That means each command list allocated from this allocator
             * must be recorded in order on the same thread. The command allocator and command lists
             * are returned to the pool on @see Reset.
             */
            class CommandListSubAllocator final
            {
            public:
                CommandListSubAllocator() = default;
                CommandListSubAllocator(const CommandListSubAllocator&) = delete;

                void Init(
                    CommandAllocatorPool& commandAllocatorPool,
                    CommandListPool& commandListPool);

                CommandList* Allocate();

                void Reset();

            private:
                ID3D12CommandAllocator* m_currentAllocator = nullptr;
                CommandAllocatorPool* m_commandAllocatorPool = nullptr;
                CommandListPool* m_commandListPool = nullptr;
                AZStd::vector<CommandList*> m_activeLists;
            };

            ///////////////////////////////////////////////////////////////////
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
         */
        class CommandListAllocator final
        {
        public:
            CommandListAllocator() = default;

            struct Descriptor
            {
                // The device used for creating the command lists.
                Device* m_device = nullptr;

                // The maximum number of frames to keep buffered on the CPU timeline.
                uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;

                // The DX12 descriptor context used to allocate and map views for command lists.
                AZStd::shared_ptr<DescriptorContext> m_descriptorContext;
            };

            void Init(const Descriptor& descriptor);

            void Shutdown();

            // Allocates a new command list on the current thread for the given
            // hardware queue. Each command list allocated per thread, per queue must be
            // recorded and closed in the order they were acquired.
            CommandList* Allocate(RHI::HardwareQueueClass hardwareQueueClass);

            // Call this once per frame to retire the current frame and reclaim
            // elements from completed frames
            void Collect();

        private:
            bool m_isInitialized = false;
            AZStd::array<Internal::CommandListPool, RHI::HardwareQueueClassCount> m_commandListPools;
            AZStd::array<Internal::CommandAllocatorPool, RHI::HardwareQueueClassCount> m_commandAllocatorPools;
            AZStd::array<RHI::ThreadLocalContext<Internal::CommandListSubAllocator>, RHI::HardwareQueueClassCount> m_commandListSubAllocators;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/unordered_map.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSet.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        namespace Internal
        {
            class DescriptorPoolFactory final
                : public RHI::ObjectFactoryBase<DescriptorPool>
            {
                using Base = RHI::ObjectFactoryBase<DescriptorPool>;
            public:
                struct Descriptor
                {
                    Device* m_device = nullptr;
                };

                void Init(const Descriptor& descriptor);

                RHI::Ptr<DescriptorPool> CreateObject(const DescriptorPool::Descriptor& poolDescriptor);

                void ResetObject(DescriptorPool& descriptorPool, const DescriptorPool::Descriptor& poolDescriptor);
                void ShutdownObject(DescriptorPool& descriptorPool, bool isPoolShutdown);
                bool CollectObject(DescriptorPool& descriptorPool);

                const Descriptor& GetDescriptor() const;

            private:
                Descriptor m_descriptor;
            };

            struct DescriptorPoolAllocatorlTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = DescriptorPool;
                using ObjectFactoryType = DescriptorPoolFactory;
                using MutexType = RHI::NullMutex;
            };

            using DescriptorPoolAllocator = RHI::ObjectPool<DescriptorPoolAllocatorlTraits>;

            class DescriptorSetSubAllocator final
            {
                using ObjectType = DescriptorSet;
            public:
                DescriptorSetSubAllocator() = default;
                DescriptorSetSubAllocator(const DescriptorSetSubAllocator&) = delete;

                void Init(DescriptorPoolAllocator& descriptorPoolAllocator, Device& device, const DescriptorPool::Descriptor& poolDescriptor);

                RHI::Ptr<ObjectType> Allocate(DescriptorSetLayout& layout);
                void DeAllocate(RHI::Ptr<ObjectType> descriptorSet);
                void Reset();
                void Collect();

            private:
                Device* m_device;
                DescriptorPoolAllocator* m_descriptorPoolAllocator = nullptr;
                DescriptorPool::Descriptor m_poolDescriptor;
                AZStd::list<DescriptorPool*> m_pools;
            };
        }

        /**
        * Allocator for creating descriptor sets.
        * Each descriptor set is allocated from a descriptor set pool.
        * When the pool can't allocate more descriptor sets we create a new pool.
        * We use the return value from Vulkan to check if the pool ran out of memory and we need to create
        * a new one. A DescriptorSetAllocator is used to generate new descriptor set pools when needed.
        */
        class DescriptorSetAllocator final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            using ObjectType = DescriptorSet;

        public:
            AZ_CLASS_ALLOCATOR(DescriptorSetAllocator, AZ::SystemAllocator);

            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
                uint32_t m_poolSize = 0;
                const DescriptorSetLayout* m_layout = nullptr;
                BufferPool* m_constantDataPool = nullptr;
            };

            DescriptorSetAllocator() = default;
            ~DescriptorSetAllocator() = default;

            RHI::ResultCode Init(const Descriptor& descriptor);
            RHI::Ptr<ObjectType> Allocate(DescriptorSetLayout& layout);
            void DeAllocate(RHI::Ptr<ObjectType> descriptor);
            void Collect();
            void Shutdown() override;

        private:
            AZStd::mutex m_subAllocatorMutex;
            Internal::DescriptorSetSubAllocator m_subAllocator;
            Internal::DescriptorPoolAllocator m_poolAllocator;
            Descriptor m_descriptor;
            bool m_isInitialized = false;
        };
    }
}

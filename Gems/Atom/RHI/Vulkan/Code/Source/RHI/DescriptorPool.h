/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/ObjectCollector.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/DescriptorSet.h>
#include <RHI/ReleaseQueue.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroupLayout;
    }

    namespace Vulkan
    {
        class Device;
        class DescriptorSetLayout;
        class BufferPool;
        namespace Internal
        {
            class DescriptorPoolFactory;
        }

        class DescriptorPool final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            using ObjectType = DescriptorSet;
            friend class Internal::DescriptorPoolFactory;
            friend class BindlessDescriptorPool;

        public:
            AZ_CLASS_ALLOCATOR(DescriptorPool, AZ::ThreadPoolAllocator);
            AZ_RTTI(DescriptorPool, "AB200FE2-5783-4BB7-9FDB-A99C3CDC1161", Base);

            struct Descriptor
            {
                Device* m_device = nullptr;
                AZStd::vector<VkDescriptorPoolSize> m_descriptorPoolSizes;
                uint32_t m_maxSets = 0;
                uint32_t m_collectLatency = 0;
                BufferPool* m_constantDataPool = nullptr;
                bool m_updateAfterBind = false;
            };

            ~DescriptorPool();

            using AllocResult = AZStd::pair<VkResult, RHI::Ptr<ObjectType>>;

            AllocResult Allocate(const DescriptorSetLayout& descriptorSetLayout);
            void DeAllocate(RHI::Ptr<ObjectType> object);
            const Descriptor& GetDescriptor() const;
            VkDescriptorPool GetNativeDescriptorPool() const;

            // Return the total number of objects in the pool. This include the pool objects +
            // the ones queued for deletion.
            size_t GetTotalObjectCount() const;
            void Collect();

        private:
            DescriptorPool() = default;
            static RHI::Ptr<DescriptorPool> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);
            void Reset();

            RHI::ResultCode BuildNativeDescriptorPool();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            Descriptor m_descriptor;
            VkDescriptorPool m_nativeDescriptorPool = VK_NULL_HANDLE;
            ReleaseQueue m_collector;
            AZStd::unordered_set<RHI::Ptr<ObjectType>> m_objects;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <RHI/Memory.h>

#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        //! Factory used by the MemoryPageAllocator for creating Memory Page objects.
        class MemoryPageFactory
            : public RHI::ObjectFactoryBase<Memory>
        {
        public:
            using GetHeapMemoryUsageFunction = AZStd::function<RHI::HeapMemoryUsage*()>;

            struct Descriptor
            {
                Device* m_device = nullptr;
                size_t m_pageSizeInBytes = 0;
                RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                RHI::HostMemoryAccess m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                VkMemoryPropertyFlags m_additionalMemoryPropertyFlags = 0;
                uint32_t m_memoryTypeBits = 0;
                GetHeapMemoryUsageFunction m_getHeapMemoryUsageFunction;
                bool m_recycleOnCollect = true;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<Memory> CreateObject();

            RHI::Ptr<Memory> CreateObject(size_t sizeInBytes);

            void ShutdownObject(Memory& object, bool isPoolShutdown = false);

            bool CollectObject(Memory& object);

            const Descriptor& GetDescriptor() const;

            void SetDebugName(const AZStd::string_view& name) const;

        private:
            Device& GetDevice() const;

            Descriptor m_descriptor;
            mutable AZ::Name m_debugName;
        };

        class MemoryPageAllocatorTraits
            : public RHI::ObjectPoolTraits
        {
        public:
            using ObjectType = Memory;
            using ObjectFactoryType = MemoryPageFactory;
            using MutexType = AZStd::mutex;
        };

        class MemoryPageAllocator
            : public RHI::ObjectPool<MemoryPageAllocatorTraits>
        {
        public:
            size_t GetPageCount() const
            {
                return GetObjectCount();
            }

            size_t GetPageSize() const
            {
                return GetFactory().GetDescriptor().m_pageSizeInBytes;
            }

            void SetName(const AZStd::string_view& name)
            {
                GetFactory().SetDebugName(name);
            }
        };
    }
}

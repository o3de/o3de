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

namespace AZ
{
    namespace Metal
    {
        class Device;

        class MemoryPageFactory
            : public RHI::ObjectFactoryBase<Memory>
        {
        public:
            using GetHeapMemoryUsageFunction = AZStd::function<RHI::HeapMemoryUsage*()>;

            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_pageSizeInBytes = 0;
                RHI::BufferBindFlags m_bindFlags = RHI::BufferBindFlags::None;
                RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                RHI::HostMemoryAccess m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                GetHeapMemoryUsageFunction m_getHeapMemoryUsageFunction;
                // we want to release the heap page when the tile allocator de-allocates it
                bool m_recycleOnCollect = false;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<Memory> CreateObject();

            void ShutdownObject(Memory& object, bool isPoolShutdown);

            bool CollectObject(Memory& object);

            const Descriptor& GetDescriptor() const;

        private:
            Descriptor m_descriptor;
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
        };
    }
}

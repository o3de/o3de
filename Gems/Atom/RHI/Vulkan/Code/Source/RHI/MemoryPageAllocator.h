/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

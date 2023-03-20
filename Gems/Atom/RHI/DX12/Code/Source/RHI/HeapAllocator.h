/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Allocator.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/PoolAllocator.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>

#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
                
        using Heap = ID3D12Heap;

        enum class ResourceType : uint32_t
        {
            Image = 0,
            RenderTarget,
            Buffer,
            Count
        };

        //! Flags to describe the resources supported by a heap.
        enum class ResourceTypeFlags : uint32_t
        {
            Image = AZ_BIT(static_cast<uint32_t>(ResourceType::Image)),
            RenderTarget = AZ_BIT(static_cast<uint32_t>(ResourceType::RenderTarget)),
            Buffer = AZ_BIT(static_cast<uint32_t>(ResourceType::Buffer)),
            All = Image | RenderTarget | Buffer
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::DX12::ResourceTypeFlags)

        //! Factory which is responsible for allocating heap pages from the GPU
        class HeapFactory
            : public RHI::ObjectFactoryBase<Heap>
        {
        public:

            using GetHeapMemoryUsageFunction = AZStd::function<RHI::HeapMemoryUsage*()>;
            struct Descriptor
            {
                Device* m_device = nullptr;
                uint32_t m_pageSizeInBytes = 0;
                ResourceTypeFlags m_resourceTypeFlags = ResourceTypeFlags::Image;
                RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                RHI::HostMemoryAccess m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                bool m_recycleOnCollect = false;    // we want to release the heap page when the tile allocator de-allocates it

                GetHeapMemoryUsageFunction m_getHeapMemoryUsageFunction;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<Heap> CreateObject();

            void ShutdownObject(Heap& object, bool isPoolShutdown);

            bool CollectObject(Heap& object);

            const Descriptor& GetDescriptor() const;

        private:
            Descriptor m_descriptor;
            D3D12_HEAP_FLAGS m_heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_HEAP_TYPE m_heapType;
        };

        class HeapAllocatorTraits
            : public RHI::ObjectPoolTraits
        {
        public:
            using ObjectType = Heap;
            using ObjectFactoryType = HeapFactory;
            using MutexType = AZStd::mutex;
        };

        class HeapAllocator
            : public RHI::ObjectPool<HeapAllocatorTraits>
        {
        public:
            size_t GetPageCount() const
            {
                return GetObjectCount();
            }

            uint32_t GetPageSize() const
            {
                return GetFactory().GetDescriptor().m_pageSizeInBytes;
            }
        };
    }
}

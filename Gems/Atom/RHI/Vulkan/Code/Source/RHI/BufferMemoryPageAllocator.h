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
#include <RHI/BufferMemory.h>
#include <RHI/MemoryPageAllocator.h>

#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        //! Factory used by the BufferMemoryPageAllocator for creating BufferMemory Page objects.
        class BufferMemoryPageFactory
            : public RHI::ObjectFactoryBase<BufferMemory>
        {
        public:

            struct Descriptor : MemoryPageFactory::Descriptor
            {
                RHI::BufferBindFlags m_bindFlags = RHI::BufferBindFlags::None;
                RHI::HardwareQueueClassMask m_sharedQueueMask = RHI::HardwareQueueClassMask::All;
            };

            void Init(const Descriptor& descriptor);

            RHI::Ptr<BufferMemory> CreateObject();

            RHI::Ptr<BufferMemory> CreateObject(size_t sizeInBytes);

            void ShutdownObject(BufferMemory& object, bool isPoolShutdown = false);

            bool CollectObject(BufferMemory& object);

            const Descriptor& GetDescriptor() const;

            void SetDebugName(const AZStd::string_view& name) const;

        private:
            Device& GetDevice() const;

            Descriptor m_descriptor;
            mutable AZ::Name m_debugName;
        };

        class BufferMemoryPageAllocatorTraits
            : public RHI::ObjectPoolTraits
        {
        public:
            using ObjectType = BufferMemory;
            using ObjectFactoryType = BufferMemoryPageFactory;
            using MutexType = AZStd::mutex;
        };

        class BufferMemoryPageAllocator
            : public RHI::ObjectPool<BufferMemoryPageAllocatorTraits>
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

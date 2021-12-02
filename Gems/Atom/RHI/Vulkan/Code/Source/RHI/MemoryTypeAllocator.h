/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <RHI/Device.h>
#include <RHI/MemoryTypeView.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        //! This class is a wrapper over the "SubAllocator" class. Besides of exposing the
        //! "SubAllocator" allocation functions (alloc, dealloc, etc), it provides the functionality of allocating 
        //! memory objects that are bigger than the "SubAllocator" page (a "unique" allocation), it creates and holds
        //! the PageAllocator object, it provides thread safe access to the "SubAllocator" and it wraps allocations      
        //! in a "View" object.
        //! Allocations are thread safe.
        template<typename SubAllocator, typename View>
        class MemoryTypeAllocator :
            public RHI::DeviceObject
        {
        public:
            using PageAllocator = typename SubAllocator::page_allocator_type;
            using Descriptor = typename PageAllocator::Descriptor;
            using ViewBase = MemoryTypeView<typename SubAllocator::memory_type>;

            MemoryTypeAllocator() = default;
            MemoryTypeAllocator(const MemoryTypeAllocator&) = delete;

            void Init(const Descriptor& descriptor);

            void Shutdown() override;

            void GarbageCollect();

            View Allocate(size_t sizeInBytes, size_t alignmentInBytes, bool forceUnique = false);

            void DeAllocate(const View& memory);

            const Descriptor& GetDescriptor() const;

        private:
            View AllocateUnique(const uint64_t sizeInBytes);

            void DeAllocateUnique(const View& memoryView);

            // RHI::Object overrides...
            void SetNameInternal(const AZStd::string_view& name) override;

            Descriptor m_descriptor;
            PageAllocator m_pageAllocator;
            AZStd::mutex m_subAllocatorMutex;
            SubAllocator m_subAllocator;
        };

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::Init(const Descriptor& descriptor)
        {
            DeviceObject::Init(*descriptor.m_device);
            m_descriptor = descriptor;
            const RHI::HeapMemoryUsage* heapMemoryUsage = descriptor.m_getHeapMemoryUsageFunction();

            const size_t budgetInBytes = heapMemoryUsage->m_budgetInBytes;
            if (budgetInBytes)
            {
                // The buffer page size should not exceed the budget.
                m_descriptor.m_pageSizeInBytes = static_cast<uint32_t>(AZStd::min<size_t>(budgetInBytes, m_descriptor.m_pageSizeInBytes));
            }

            m_pageAllocator.Init(m_descriptor);

            typename SubAllocator::Descriptor pageAllocatorDescriptor;
            pageAllocatorDescriptor.m_alignmentInBytes = SubAllocator::Descriptor::DefaultAlignment;
            pageAllocatorDescriptor.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            pageAllocatorDescriptor.m_inactivePageCycles = 1;
            m_subAllocator.Init(pageAllocatorDescriptor, m_pageAllocator);

            SetName(GetName());
        }

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::Shutdown()
        {
            m_subAllocator.Shutdown();
            m_pageAllocator.Shutdown();
        }

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::GarbageCollect()
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_subAllocatorMutex);
                m_subAllocator.GarbageCollect();
            }

            m_pageAllocator.Collect();
        }

        template<typename SubAllocator, typename View>
        View MemoryTypeAllocator<SubAllocator, View>::Allocate(size_t sizeInBytes, size_t alignmentInBytes, bool forceUnique /*=false*/)
        {
            AZ_TRACE_METHOD();


            View memoryView;

            // First attempt to sub-allocate a buffer from the sub-allocator.
            if (!forceUnique)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_subAllocatorMutex);
                memoryView = View(ViewBase(m_subAllocator.Allocate(sizeInBytes, alignmentInBytes), MemoryAllocationType::SubAllocated));
            }

            // Next, try a unique buffer allocation.
            if (!memoryView.IsValid())
            {
                memoryView = AllocateUnique(sizeInBytes);
            }

            return memoryView;
        }

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::DeAllocate(const View& memoryView)
        {
            switch (memoryView.GetAllocationType())
            {
            case MemoryAllocationType::SubAllocated:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_subAllocatorMutex);
                m_subAllocator.DeAllocate(memoryView.GetAllocation());
                break;
            }
            case MemoryAllocationType::Unique:
                DeAllocateUnique(memoryView);
                break;
            }
        }

        template<typename SubAllocator, typename View>
        const typename MemoryTypeAllocator<SubAllocator, View>::Descriptor& MemoryTypeAllocator<SubAllocator, View>::GetDescriptor() const
        {
            return m_descriptor;
        }

        template<typename SubAllocator, typename View>
        View MemoryTypeAllocator<SubAllocator, View>::AllocateUnique(const uint64_t sizeInBytes)
        {
            AZ_TRACE_METHOD();
            auto memory = const_cast<typename PageAllocator::ObjectFactoryType&>(m_pageAllocator.GetFactory()).CreateObject(sizeInBytes);
            if (!memory)
            {
                return View();
            }

            const char* name = GetName().IsEmpty() ? "Unique Allocation" : GetName().GetCStr();
            memory->SetName(Name{ name });
            return View(ViewBase(memory, 0, sizeInBytes, 0, MemoryAllocationType::Unique));
        }

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::DeAllocateUnique(const View& memoryView)
        {
            AZ_Assert(memoryView.GetAllocationType() == MemoryAllocationType::Unique, "This call only supports unique MemoryView allocations.");
            auto memory = memoryView.GetAllocation().m_memory;
            const_cast<typename PageAllocator::ObjectFactoryType&>(m_pageAllocator.GetFactory()).ShutdownObject(*memory);
            static_cast<Device&>(GetDevice()).QueueForRelease(memory);
        }

        template<typename SubAllocator, typename View>
        void MemoryTypeAllocator<SubAllocator, View>::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                m_pageAllocator.SetName(name);
            }
        }
    }
}


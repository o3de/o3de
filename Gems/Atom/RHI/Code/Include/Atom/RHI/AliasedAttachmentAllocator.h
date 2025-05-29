/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AliasedHeap.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/ObjectCollector.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::RHI
{
    class DeviceResource;
    class Scope;
    class DeviceBuffer;
    class DeviceImage;

    namespace Internal
    {
        //! Stub AliasingBarrierTracker used for the NoAllocationAliasedHeap.
        class NoBarrierAliasingBarrierTracker
            : public RHI::AliasingBarrierTracker
        {
        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasingBarrierTracker
            void AppendBarrierInternal([[maybe_unused]] const RHI::AliasedResource& resourceBefore, [[maybe_unused]] const RHI::AliasedResource& resourceAfter) override {}
            //////////////////////////////////////////////////////////////////////////
        };

        //! AliasedHeap that doesn't allocate any resources.
        //! Used to track memory usage by the allocation of transient attachments.
        class NoAllocationAliasedHeap
            : public AliasedHeap
        {
        public:
            using Descriptor = AliasedHeapDescriptor;

        private:
            //////////////////////////////////////////////////////////////////////////
            // AliasedHeap
            AZStd::unique_ptr<AliasingBarrierTracker> CreateBarrierTrackerInternal() override           { return AZStd::make_unique<NoBarrierAliasingBarrierTracker>(); }
            ResultCode InitInternal([[maybe_unused]] Device& device, [[maybe_unused]] const AliasedHeapDescriptor& descriptor) override   { return ResultCode::Success; }
            ResultCode InitImageInternal([[maybe_unused]] const DeviceImageInitRequest& request, [[maybe_unused]] size_t heapOffset) override   { return ResultCode::Success; }
            ResultCode InitBufferInternal([[maybe_unused]] const DeviceBufferInitRequest& request, [[maybe_unused]] size_t heapOffset) override { return ResultCode::Success; }
            //////////////////////////////////////////////////////////////////////////
        };
    }

    //! Utility class that Allocates Aliased Transient Attachments using one or multiple Heaps.
    //! The allocator uses pages, where each page corresponds to a Heap that is implemented by the platform.
    //! Users must provide a "Heap" class that inherits from the RHI::AliasedHeap class.
    //! This allocator use different allocation strategies described by the RHI::HeapAllocationStrategy enum.
    //! Depending on the strategy selected, the allocator can grow/shrink by allocating/deallocating heap pages.
    //! It can also compact a heap page if it is being underutilized.
    //! We also use the template type so we can inherit the AliasedAttachmentAllocator::Descriptor from the
    //! Heap::Descriptor to pass implementation specific parameters during the heap initialization.
    template<class Heap>
    class AliasedAttachmentAllocator
        : public DeviceObject
        , public FrameEventBus::Handler
    {
        static_assert(AZStd::is_base_of<AliasedHeap, Heap>::value, "Type must inherit from RHI::AliasedHeap to be used by the RHI::AliasedAttachmentAllocator.");
        using Base = DeviceObject;
    public:
        AZ_CLASS_ALLOCATOR(AliasedAttachmentAllocator<Heap>, AZ::SystemAllocator);

        struct Descriptor
            : public Heap::Descriptor
        {
            AZ_CLASS_ALLOCATOR(Descriptor, AZ::SystemAllocator)
            HeapAllocationParameters m_allocationParameters;
        };

        static Ptr<AliasedAttachmentAllocator> Create()
        {
            return aznew AliasedAttachmentAllocator<Heap>();
        }

        virtual ~AliasedAttachmentAllocator() = default;

        //! Initializes the allocator
        ResultCode Init(Device& device, const Descriptor& descriptor);

        //! This is called at the beginning of the compile phase for the current frame,
        //! before any allocations occur.
        //! @param compileFlags The flags that will be use during the resources activation.
        //! @param memoryUsageHint [Optional] The total amount of memory needed by the allocator for allocating resources in the Begin/End cycle.
        //!                        This value is needed when the allocator is using a "MemoryHint" allocation strategy.
        //!                        The amount of memory needed can be calculated by doing a first pass with the flag TransientAttachmentPoolCompileFlags::DontAllocateResources.
        void Begin(TransientAttachmentPoolCompileFlags compileFlags, const size_t memoryUsageHint = 0);

        //! Called when a buffer is being activated for the first time. This will acquire
        //! a buffer from a heap, configured for the provided descriptor. This may trigger a new
        //! heap to be allocated.
        DeviceBuffer* ActivateBuffer(
            const TransientBufferDescriptor& descriptor,
            Scope& scope);

        //! Called when a buffer is being de-allocated from the allocator. Called during the last scope the attachment
        //! is used, after all allocations for that scope have been processed.
        void DeactivateBuffer(
            const AttachmentId& attachmentId,
            Scope& scope);

        //! Called when an image is being activated for the first time. This will acquire
        //! an image from a heap, configured for the provided descriptor. This may trigger a new
        //! heap to be allocated.
        DeviceImage* ActivateImage(
            const TransientImageDescriptor& descriptor,
            Scope& scope);

        //! Called when a image is being de-allocated from the allocator. Called during the last scope the attachment
        //! is used, after all allocations for that scope have been processed.
        void DeactivateImage(
            const AttachmentId& attachmentId,
            Scope& scope);

        //! Called when the allocations / deallocations have completed for all scopes.
        void End();

        //! DeviceObject override.
        void Shutdown() override;

        //! Get statistics for the pool (built during End).
        //! Statistics will be added at the end of the provided vector.
        void GetStatistics(AZStd::vector<TransientAttachmentStatistics::Heap>& heapStatistics) const;

        //! Get allocator descriptor.
        const Descriptor& GetDescriptor() const { return m_descriptor; }

    private:
        AliasedAttachmentAllocator() = default;

        struct HeapPage
        {
            Ptr<AliasedHeap> m_heap;
            // Number of garbage collect iterations that this heap page has gone through.
            uint32_t m_collectIteration = 0;
        };

        //////////////////////////////////////////////////////////////////////////
        // FrameEventBus::Handler
        void OnFrameEnd() override;
        //////////////////////////////////////////////////////////////////////////

        // Adds a new heap page to the allocator of the provided size.
        AliasedHeap* AddAliasedHeapPage(size_t sizeInBytes, uint32_t heapIndex = 0);

        // Calculates the size of a new page depending on the strategy of the allocator.
        // The heap must at least have "minSizeInBytes" size.
        size_t CalculateHeapPageSize(size_t minSizeInBytes) const;

        // Returns the heap scale factor depending on the strategy of the allocator.
        float GetHeapPageScaleFactor() const;

        // Erase unused pages and replace pages with high unused space for a smaller one.
        // Deleted pages are garbage collect according to the garbage collect latency.
        void CompactHeapPages();

        // Iterates through all heap pages in the allocator.
        void ForEachHeap(const AZStd::function<void(AliasedHeap&)>& callback);
        void ForEachHeap(const AZStd::function<void(const AliasedHeap&)>& callback) const;

        Descriptor m_descriptor;
        AZStd::vector<HeapPage> m_heapPages;
        ObjectCollector<> m_garbageCollector;
        // The no allocation heap is used for calculating the "extra" memory that is needed to allocate
        // the attachments that don't fit the current heap pages. As the name suggests, it doesn't really allocate any resource.
        Internal::NoAllocationAliasedHeap m_noAllocationHeap;
        AZStd::unordered_map<AttachmentId, AliasedHeap*> m_attachmentToHeapMap;
        HeapMemoryUsage m_memoryUsage;

        size_t m_memoryUsageHint = 0;
        TransientAttachmentPoolCompileFlags m_compileFlags = TransientAttachmentPoolCompileFlags::None;
    };

    template<class Heap>
    size_t AliasedAttachmentAllocator<Heap>::CalculateHeapPageSize(size_t minSizeInBytes) const
    {
        size_t pageSize = 0;
        const HeapAllocationParameters& heapAllocationParameters = m_descriptor.m_allocationParameters;
        switch (heapAllocationParameters.m_type)
        {
        case HeapAllocationStrategy::MemoryHint:
            // In this strategy the page size is equal to the memory needed to handle all allocations of the begin/end cycle.
            // We know how much memory is needed thanks to the hint provided.
            AZ_Assert(m_memoryUsageHint, "No memory hint provided for aliased allocator %s", GetName().GetCStr());
            if (m_memoryUsageHint > m_memoryUsage.m_totalResidentInBytes)
            {
                pageSize = m_memoryUsageHint - m_memoryUsage.m_totalResidentInBytes;
            }
            // Limit the size to the m_minHeapSizeInBytes provided in the descriptor.
            pageSize = AZStd::max(pageSize, static_cast<size_t>(heapAllocationParameters.m_usageHintParameters.m_minHeapSizeInBytes));
            break;
        case HeapAllocationStrategy::Paging:
            // In this strategy the page size is equal to the size provided in the descriptor.
            pageSize = heapAllocationParameters.m_pagingParameters.m_pageSizeInBytes;
            break;
        case HeapAllocationStrategy::Fixed:
        default:
            AZ_Assert(false, "Invalid heap allocation strategy (%d) when calculating page size", m_descriptor.m_allocationParameters.m_type);
            break;
        }

        // Check that the page size is not bigger than the whole budget.
        if (m_descriptor.m_budgetInBytes)
        {
            pageSize = AZStd::min(pageSize, static_cast<size_t>(m_descriptor.m_budgetInBytes));
        }
        // And finally limit the size to at least be greater than the minSizeInBytes that was passed.
        pageSize = AZStd::max(pageSize, minSizeInBytes);
        return pageSize;
    }

    template<class Heap>
    float AZ::RHI::AliasedAttachmentAllocator<Heap>::GetHeapPageScaleFactor() const
    {
        const RHI::HeapAllocationParameters& heapAllocationParameters = m_descriptor.m_allocationParameters;
        switch (heapAllocationParameters.m_type)
        {
        case HeapAllocationStrategy::MemoryHint:
            return heapAllocationParameters.m_usageHintParameters.m_heapSizeScaleFactor;
        case HeapAllocationStrategy::Paging:
        case HeapAllocationStrategy::Fixed:
        default:
            return 1.0f;
        }
    }

    template<class Heap>
    ResultCode AliasedAttachmentAllocator<Heap>::Init(Device& device, const Descriptor& descriptor)
    {
        Base::Init(device);
             
        m_descriptor = descriptor;
        m_memoryUsage.m_budgetInBytes = m_descriptor.m_budgetInBytes;

        // The no allocation heap is used when doing a 2 pass strategy.
        Internal::NoAllocationAliasedHeap::Descriptor heapAllocator;
        heapAllocator.m_alignment = descriptor.m_alignment;
        heapAllocator.m_budgetInBytes = std::numeric_limits<AZ::u64>::max();
        m_noAllocationHeap.SetName(AZ::Name("AliasedAttachment_NoAllocationHeap"));
        m_noAllocationHeap.Init(device, heapAllocator);

        typename decltype(m_garbageCollector)::Descriptor collectorDescriptor;
        collectorDescriptor.m_collectLatency = Limits::Device::FrameCountMax;
        collectorDescriptor.m_collectFunction = [this](Object& object)
        {
            AliasedHeap& heap = static_cast<AliasedHeap&>(object);
            m_memoryUsage.m_totalResidentInBytes -= heap.GetDescriptor().m_budgetInBytes;
        };
        m_garbageCollector.Init(collectorDescriptor);

        size_t initialHeapSize = 0;
        switch (descriptor.m_allocationParameters.m_type)
        {
        case HeapAllocationStrategy::Paging:
            initialHeapSize = static_cast<size_t>(m_descriptor.m_budgetInBytes * descriptor.m_allocationParameters.m_pagingParameters.m_initialAllocationPercentage);
            break;
        case HeapAllocationStrategy::Fixed:
            initialHeapSize = m_descriptor.m_budgetInBytes;
            break;
        case HeapAllocationStrategy::MemoryHint:
            initialHeapSize = 0;
            break;
        default:
            AZ_Assert(false, "Invalid heap allocation strategy %d", descriptor.m_allocationParameters.m_type);
            return ResultCode::InvalidArgument;
        }

        if (initialHeapSize)
        {
            if (!AddAliasedHeapPage(initialHeapSize))
            {
                AZ_Assert(false, "Failed to create initial heap");
                return ResultCode::Fail;
            }
        }

        FrameEventBus::Handler::BusConnect(&device);
             
        return ResultCode::Success;
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::Begin(TransientAttachmentPoolCompileFlags compileFlags, const size_t memoryUsageHint /*= 0*/)
    {
        m_memoryUsageHint = memoryUsageHint;
        m_compileFlags = compileFlags;
        ForEachHeap([&compileFlags](AliasedHeap& heap)
        {
            heap.Begin(compileFlags);
        });

        if (CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            m_noAllocationHeap.Begin(compileFlags);
        }
    }

    template<class Heap>
    DeviceBuffer* AliasedAttachmentAllocator<Heap>::ActivateBuffer(const TransientBufferDescriptor& descriptor, Scope& scope)
    {
        DeviceBuffer* buffer = nullptr;
        AliasedHeap* heap = nullptr;
        ResultCode result = ResultCode::Fail;
        // We first try to allocate from the current heap pages.
        // When running with the TransientAttachmentPoolCompileFlags::DontAllocateResources flag
        // the heaps will not create any resources but we need then to "allocate" the space needed.
        for (const HeapPage& page : m_heapPages)
        {
            Ptr<AliasedHeap> aliasedHeap = page.m_heap;
            result = aliasedHeap->ActivateBuffer(descriptor, scope, &buffer);
            if (result == ResultCode::Success)
            {
                heap = aliasedHeap.get();
                break;
            }
        }

        if (result != ResultCode::Success)
        {
            // If we don't have enough space in the heaps to accommodate the buffer, we need to create a new
            // heap page.
            if (CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
            {
                // When running with the TransientAttachmentPoolCompileFlags::DontAllocateResources flag we just
                // collect this "extra" memory needed in a dummy allocator so we can calculate the total memory needed at the end.
                heap = &m_noAllocationHeap;
                m_noAllocationHeap.ActivateBuffer(descriptor, scope, &buffer);
            }
            else
            {
                if (m_descriptor.m_allocationParameters.m_type != HeapAllocationStrategy::Fixed)
                {
                    ResourceMemoryRequirements memRequirements = GetDevice().GetResourceMemoryRequirements(descriptor.m_bufferDescriptor);
                    heap = AddAliasedHeapPage(CalculateHeapPageSize(memRequirements.m_sizeInBytes), aznumeric_cast<uint32_t>(m_heapPages.size()));
                    result = heap->ActivateBuffer(descriptor, scope, &buffer);
                }

                if (result != ResultCode::Success)
                {
                    AZ_Assert(false, "Failed to allocated aliased buffer");
                    return nullptr;
                }
            }
        }

        m_attachmentToHeapMap.insert({ descriptor.m_attachmentId, heap });
        return buffer;
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::DeactivateBuffer(const AttachmentId& attachmentId, Scope& scope)
    {
        auto findIter = m_attachmentToHeapMap.find(attachmentId);
        if (findIter == m_attachmentToHeapMap.end())
        {
            AZ_Assert(false, "Failed to find aliased buffer %s when deactivating", attachmentId.GetCStr());
            return;
        }

        findIter->second->DeactivateBuffer(attachmentId, scope);
        m_attachmentToHeapMap.erase(findIter);
    }

    template<class Heap>
    DeviceImage* AliasedAttachmentAllocator<Heap>::ActivateImage(const TransientImageDescriptor& descriptor, Scope& scope)
    {
        DeviceImage* image = nullptr;
        AliasedHeap* heap = nullptr;
        ResultCode result = ResultCode::Fail;
        // We first try to allocate from the current heap pages.
        // When running with the TransientAttachmentPoolCompileFlags::DontAllocateResources flag
        // the heaps will not create any resources but we need then to "allocate" the space needed.
        for (const HeapPage& page : m_heapPages)
        {
            Ptr<AliasedHeap> aliasedHeap = page.m_heap;
            result = aliasedHeap->ActivateImage(descriptor, scope, &image);
            if (result == ResultCode::Success)
            {
                heap = aliasedHeap.get();
                break;
            }
        }

        if (result != ResultCode::Success)
        {
            // If we don't have enough space in the heaps to accommodate the image, we need to create a new
            // heap page.
            if (CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
            {
                // When running with the TransientAttachmentPoolCompileFlags::DontAllocateResources flag we just
                // collect this "extra" memory needed in a dummy allocator so we can calculate the total memory needed at the end.
                heap = &m_noAllocationHeap;
                m_noAllocationHeap.ActivateImage(descriptor, scope, &image);
            }
            else
            {
                // In a fixed strategy we never allocate new heap pages.
                if (m_descriptor.m_allocationParameters.m_type != HeapAllocationStrategy::Fixed)
                {
                    ResourceMemoryRequirements memRequirements = GetDevice().GetResourceMemoryRequirements(descriptor.m_imageDescriptor);
                    heap = AddAliasedHeapPage(CalculateHeapPageSize(memRequirements.m_sizeInBytes), aznumeric_cast<uint32_t>(m_heapPages.size()));
                    AZ_Assert(heap, "Failed to allocated aliased heap page");
                    if (!heap)
                    {
                        return nullptr;
                    }
                    result = heap->ActivateImage(descriptor, scope, &image);
                }

                if (result != ResultCode::Success)
                {
                    AZ_Assert(false, "Failed to allocated aliased image");
                    return nullptr;
                }
            }
        }

        m_attachmentToHeapMap.insert({ descriptor.m_attachmentId, heap });

        // Remove any stale resource entries made in pages other than the one where it currently resides. 
        for (const HeapPage& page : m_heapPages)
        {
            Ptr<AliasedHeap> aliasedHeap = page.m_heap;
            if (heap == aliasedHeap)
            {
                continue;
            }
            aliasedHeap->RemoveFromCache(descriptor.m_attachmentId);
        }
        return image;
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::DeactivateImage(const AttachmentId& attachmentId, Scope& scope)
    {
        auto findIter = m_attachmentToHeapMap.find(attachmentId);
        if (findIter == m_attachmentToHeapMap.end())
        {
            AZ_Assert(false, "Failed to find aliased image %s when deactivating", attachmentId.GetCStr());
            return;
        }

        findIter->second->DeactivateImage(attachmentId, scope);
        m_attachmentToHeapMap.erase(findIter);
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::End()
    {
        ForEachHeap([](AliasedHeap& aliasedHeap)
        {
            aliasedHeap.End();
        });
        m_noAllocationHeap.End();

        if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            CompactHeapPages();

            if (m_memoryUsage.m_budgetInBytes && m_memoryUsage.m_totalResidentInBytes > m_memoryUsage.m_budgetInBytes)
            {
                // Log an error to let the user know they are going over the budget.
                AZ_Error(
                    "AliasedAttachmentAllocator",
                    false,
                    "Going over the budget for aliased heap %s. Budget: %d. Current: %d. Please increase the memory budget or decrease memory usage for the heap",
                    GetName().GetCStr(),
                    m_memoryUsage.m_budgetInBytes,
                    m_memoryUsage.m_totalResidentInBytes.load());
            }
        }
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::Shutdown()
    {
        m_attachmentToHeapMap.clear();
        m_heapPages.clear();
        m_garbageCollector.Shutdown();
        m_noAllocationHeap.Shutdown();
        FrameEventBus::Handler::BusDisconnect();
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::GetStatistics(AZStd::vector<TransientAttachmentStatistics::Heap>& heapStatistics) const
    {
        uint32_t heapIndex = 0;
        ForEachHeap([&heapStatistics, &heapIndex, this](const AliasedHeap& aliasedHeap)
        {
            heapStatistics.push_back(aliasedHeap.GetStatistics());
            heapStatistics.back().m_name = AZ::Name(AZStd::string::format("%s - Heap %d", GetName().GetCStr(), ++heapIndex));
        });

        if (CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            // Set the no allocation heap to the watermark size to collect the exact "extra" size needed.
            TransientAttachmentStatistics::Heap noAllocationStats = m_noAllocationHeap.GetStatistics();
            noAllocationStats.m_heapSize = noAllocationStats.m_watermarkSize;
            heapStatistics.push_back(noAllocationStats);
        }
    }

    template<class Heap>
    AliasedHeap* AliasedAttachmentAllocator<Heap>::AddAliasedHeapPage(size_t sizeInBytes, uint32_t heapIndex)
    {
        size_t newHeapSize = static_cast<size_t>(AlignUp(GetHeapPageScaleFactor() * sizeInBytes, m_descriptor.m_alignment));

        typename Heap::Descriptor heapDescriptor = m_descriptor;
        heapDescriptor.m_budgetInBytes = newHeapSize;
        Ptr<AliasedHeap> newHeap = Heap::Create();
        newHeap->SetName(Name(AZStd::string::format("%s_Page%i", GetName().GetCStr(), heapIndex)));
        ResultCode result = newHeap->Init(GetDevice(), heapDescriptor);
        if (result != ResultCode::Success)
        {
            return nullptr;
        }

        m_memoryUsage.m_totalResidentInBytes += newHeapSize;
        m_heapPages.push_back({ newHeap, 0 });
        return newHeap.get();
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::CompactHeapPages()
    {
        float maxWastedPercentage = 1.0;
        size_t minHeapSize = 0;
        uint32_t collectLatency = 0;
        switch (m_descriptor.m_allocationParameters.m_type)
        {
        case HeapAllocationStrategy::MemoryHint:
        {
            const HeapMemoryHintParameters& allocParameters = m_descriptor.m_allocationParameters.m_usageHintParameters;
            maxWastedPercentage = allocParameters.m_maxHeapWastedPercentage;
            collectLatency = allocParameters.m_collectLatency;
            minHeapSize = allocParameters.m_minHeapSizeInBytes;
            break;
        }
        case HeapAllocationStrategy::Paging:
        {
            const HeapPagingParameters& allocParameters = m_descriptor.m_allocationParameters.m_pagingParameters;
            maxWastedPercentage = 1.0f;
            collectLatency = allocParameters.m_collectLatency;
            minHeapSize = allocParameters.m_pageSizeInBytes;
            break;
        }
        default:
            return;
        }
            
        for (uint32_t i = 0; i < m_heapPages.size();)
        {
            HeapPage& heapPage = m_heapPages[i];
            AliasedHeap* heap = heapPage.m_heap.get();
            size_t watermarkSize = heap->GetStatistics().m_watermarkSize;
            size_t heapSize = heap->GetDescriptor().m_budgetInBytes;
            size_t wastedSpace = heapSize - watermarkSize;
            bool isEmpty = watermarkSize == 0;
            float wastedPercentage = static_cast<float>(wastedSpace) / heapSize;
            if ((wastedPercentage >= maxWastedPercentage) &&
                (heapSize > minHeapSize || isEmpty)) // Don't deallocate heaps that have the min size unless they are empty.
            {
                if ((heapPage.m_collectIteration++) > collectLatency)
                {                        
                    m_garbageCollector.QueueForCollect(heap);
                    if (isEmpty)
                    {
                        m_heapPages.erase(m_heapPages.begin() + i);
                        continue;
                    }
                    else
                    {
                        AddAliasedHeapPage(watermarkSize, i);
                        AZStd::iter_swap(m_heapPages.begin() + i, m_heapPages.rbegin());
                        m_heapPages.pop_back();
                    }
                }
            }
            else
            {
                heapPage.m_collectIteration = 0;
            }

            ++i;
        }
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::ForEachHeap(const AZStd::function<void(AliasedHeap&)>& callback)
    {
        AZStd::for_each(m_heapPages.begin(), m_heapPages.end(), [&callback](auto& heapPage)
        {
            callback(*heapPage.m_heap);
        });
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::ForEachHeap(const AZStd::function<void(const AliasedHeap&)>& callback) const
    {
        AZStd::for_each(m_heapPages.begin(), m_heapPages.end(), [&callback](auto& heapPage)
        {
            callback(*heapPage.m_heap);
        });
    }

    template<class Heap>
    void AliasedAttachmentAllocator<Heap>::OnFrameEnd()
    {
        m_garbageCollector.Collect();
    }
}

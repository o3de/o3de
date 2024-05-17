/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>
#include <Atom/RHI/AliasingBarrierTracker.h>
#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::RHI
{
    class DeviceResource;
    class Scope;
    class DeviceBuffer;
    class DeviceImage;

    struct AliasedHeapDescriptor
        : public ResourcePoolDescriptor
    {
        AZ_CLASS_ALLOCATOR(AliasedHeapDescriptor, SystemAllocator)
        static const uint32_t DefaultCacheSize = 256;
        static const size_t DefaultAlignment = 256;

        uint32_t m_cacheSize = DefaultCacheSize;
        size_t m_alignment = DefaultAlignment;
        AliasedResourceTypeFlags m_resourceTypeMask = AliasedResourceTypeFlags::All;
    };

    //! An Aliased Heap is a resource pool that uses a heap of memory that allow aliasing of resources.
    //! Two resources are aliased when they use the same portion of memory.
    //! Aliased Heaps are used for allocating transient attachments (resources that are valid only during the duration of a frame).
    //! and they will reuse memory whenever possible, and will also track the necessary barriers that need to be inserted when aliasing happens.
    //! Aliased Heaps do not support aliased resources being used at the same time (even if the resources are compatible).
    class AliasedHeap
        : public DeviceResourcePool
    {
        using Base = DeviceResourcePool;
    public:
        AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
        AZ_RTTI(AliasedHeap, "{9C4BB24D-3B76-4584-BA68-600BC7E2A2AA}");

        ~AliasedHeap() override = default;

        //! Initialize an Aliased Heap
        ResultCode Init(Device& device, const AliasedHeapDescriptor& descriptor);

        //! Begin the use of an Aliased Heap in a frame. Resets all previous resource uses.
        //! @param compileFlags Flags that modify behavior of the heap.
        void Begin(const RHI::TransientAttachmentPoolCompileFlags compileFlags);

        //! Begin the use of a buffer resource.
        ResultCode ActivateBuffer(
            const TransientBufferDescriptor& descriptor,
            Scope& scope,
            DeviceBuffer** activatedBuffer);

        //! Ends the use of a previously activated buffer.
        void DeactivateBuffer(
            const AttachmentId& bufferAttachment,
            Scope& scope);

        //! Begin the use of an image resource.
        ResultCode ActivateImage(
            const TransientImageDescriptor& descriptor,
            Scope& scope,
            DeviceImage** activatedImage);

        //! Ends the use of a previously activated image.
        void DeactivateImage(
            const AttachmentId& imageAttachment,
            Scope& scope);

        //! Ends the use of an Aliased Heap in a frame.
        void End();

        //! Returns the descriptor.
        const AliasedHeapDescriptor& GetDescriptor() const override;

        //! Returns the heap statistics of the frame when the Aliased Heap was began with the GatherStatistics flag.
        const TransientAttachmentStatistics::Heap& GetStatistics() const;

        //! Remove the entry related to the provided attachmentId from the cache as it is probably stale now
        void RemoveFromCache(RHI::AttachmentId attachmentId);

    protected:
        AliasedHeap() = default;

        //////////////////////////////////////////////////////////////////////////
        // Functions that must be implemented by each RHI.

        //! Creates a barrier tracker object for the Aliased Heap to use.
        virtual AZStd::unique_ptr<AliasingBarrierTracker> CreateBarrierTrackerInternal() = 0;
        //! Implementation specific initialization.
        virtual ResultCode InitInternal(Device& device, const AliasedHeapDescriptor& descriptor) = 0;
        //! Implementation initialization of an Aliased image.
        //! @param heapOffset Offset in bytes of the heap where the resource should be created.
        virtual ResultCode InitImageInternal(const DeviceImageInitRequest& request, size_t heapOffset) = 0;
        //! Implementation initialization of an Aliased buffer.
        //! @param heapOffset Offset in bytes of the heap where the resource should be created.
        virtual ResultCode InitBufferInternal(const DeviceBufferInitRequest& request, size_t heapOffset) = 0;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DeviceResourcePool
        void ShutdownInternal() override;
        void ComputeFragmentation() const override;
        //////////////////////////////////////////////////////////////////////////

    private:
        void DeactivateResourceInternal(const AttachmentId& attachmentId, Scope& scope, AliasedResourceType type);

        /// Descriptor of the heap.
        AliasedHeapDescriptor m_descriptor;

        /// First fit allocator used to allocate from placed heap.
        FreeListAllocator m_firstFitAllocator;

        /// Cache of attachments.
        ObjectCache<DeviceResource> m_cache;

        /// The aliasing barrier tracker used to compute aliasing barriers when activations
        /// and deactivations occur.
        AZStd::unique_ptr<AliasingBarrierTracker> m_barrierTracker;

        /// Tracks the total number of allocations for this cycle. This *cannot* exceed the size of the
        /// cache, or we will effectively release active resources.
        uint32_t m_totalAllocations = 0;

        /// The compile flags to use when activating / deactivating.
        TransientAttachmentPoolCompileFlags m_compileFlags = TransientAttachmentPoolCompileFlags::None;

        /// Statistics block for tracking stats (also used for book-keeping).
        RHI::TransientAttachmentStatistics::Heap m_heapStats;

        /// Reverse lookup for getting the attachment index the heap statistics.
        struct AttachmentData
        {
            DeviceResource* m_resource = nullptr;
            uint32_t m_attachmentIndex = 0;
            Scope* m_activateScope = nullptr;
        };

        AZStd::unordered_map<AttachmentId, AttachmentData> m_activeAttachmentLookup;

        // This map is used to reverse look up resource hash so we can clear them out of m_cache
        // once they have been replaced with a new resource at a different place in the heap. 
        AZStd::unordered_map<AttachmentId, HashValue64> m_reverseLookupHash;
    };
}

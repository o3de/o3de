/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::RHI
{
    class Scope;
    class DeviceResource;

    //! Describes the use of an Aliased Resource.
    struct AliasedResource
    {
        AttachmentId m_attachmentId;    //< Id of the attachment being aliased.
        Scope* m_beginScope = nullptr;  ///< Scope when the resource begins being used.
        Scope* m_endScope = nullptr;    ///< Scope when the resource ends being used.
        DeviceResource* m_resource = nullptr; ///< DeviceResource being aliased.
        uint64_t m_byteOffsetMin = 0;   ///< Begin offset in the memory heap for the aliased resource.
        uint64_t m_byteOffsetMax = 0;   ///< End offset in the memory heap for the aliased resource.
        AliasedResourceType m_type = AliasedResourceType::Image; // Type of resource being aliased.
    };

    // Tracks aliased resource and adds the proper barriers when two resources
    // overlaps each other, partially or totally. It doesn't add any type of synchronization between resources
    // that don't overlap. Resources must be added in order so the tracker knows which one is the source
    // and which one is the destination.
    class AliasingBarrierTracker
    {
    public:
        AZ_CLASS_ALLOCATOR(AliasingBarrierTracker, AZ::SystemAllocator);
        AZ_RTTI(AliasingBarrierTracker, "{2060FE50-65CB-4CC5-9FA8-0BFC9E8AF225}");
            
        virtual ~AliasingBarrierTracker() = default;

        //! Resets all previously added resource usages.
        void Reset();

        //! Add the usage of a resource in a heap.
        void AddResource(const AliasedResource& resourceNew);

        //! Signal the ends of adding resources to the tracker.
        void End();

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Functions that must be implemented by each RHI.

        //! Implementation specific AddResource. Optional.
        virtual void AddResourceInternal(const AliasedResource& resourceNew);
        //! Implementation specific reset logic. Optional.
        virtual void ResetInternal();
        //! Implementation specific end logic. Optional.
        virtual void EndInternal();
        //! Adds a barrier between to aliased resources.
        virtual void AppendBarrierInternal(const AliasedResource& resourceBefore, const AliasedResource& resourceAfter) = 0;

        //////////////////////////////////////////////////////////////////////////

    private:
        enum class Overlap
        {
            Disjoint = 0,
            Partial,
            Complete
        };

        // Returns overlap of the new against the old resource.
        Overlap GetOverlap(const AliasedResource& resourceBefore, const AliasedResource& resourceAfter) const;

        void TryAppendBarrier(const AliasedResource& resourceBefore, const AliasedResource& resourceAfter);

        AZStd::vector<AliasedResource> m_resources;
        AZStd::unordered_set<size_t> m_barrierHashes;
    };
}

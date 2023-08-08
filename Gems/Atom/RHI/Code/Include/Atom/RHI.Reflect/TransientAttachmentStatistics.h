/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/AliasedHeapEnums.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    struct TransientAttachmentStatistics
    {
        enum class AllocationPolicy
        {
            // This policy is used when the platform is using resource placement
            // onto memory heaps. This may include aliasing of memory. In this case,
            // resources will overlap on the heap at different scopes.
            HeapPlacement,

            // This policy is used when the platform is using simple object pooling.
            // In this case, the heap offsets should be ignored, and each heap instance is
            // is treated as a pool of disjoint attachments. The user can sum the total.
            ObjectPooling
        };

        struct Attachment
        {
            AttachmentId m_id;

            /// Minimum heap offset in bytes of the attachment. This will be 0 if the ObjectPooling
            /// policy is used.
            size_t m_heapOffsetMin = 0;

            /// Maximum heap offset in bytes of the attachment. This will be 0 if the ObjectPooling
            /// policy is used.
            size_t m_heapOffsetMax = 0;

            /// The index of the first scope that utilized this attachment.
            size_t m_scopeOffsetMin = 0;

            /// The index of the last scope that utilized this attachment.
            size_t m_scopeOffsetMax = 0;

            /// The size of this attachment in bytes.
            size_t m_sizeInBytes = 0;

            /// The type of the attachment.
            AliasedResourceType m_type = AliasedResourceType::RenderTarget;
        };

        struct Heap
        {
            Name m_name;

            /// The base size of the heap committed on the GPU. If the HeapPlacement policy is used,
            /// this represents a physical heap. If the ObjectPooling policy is used, it represents
            /// the total size of all attachments in the pool.
            size_t m_heapSize = 0;

            /// The watermark of allocations (simply the max of the heap offset across all scopes). If
            /// using the ObjectPooling policy, this will match the heap size.
            size_t m_watermarkSize = 0;

            /// Vector of attachments that were allocated on this heap for the previous frame.
            AZStd::vector<Attachment> m_attachments;

            //! The type of resources that the heap can allocate.
            AliasedResourceTypeFlags m_resourceTypeFlags = AliasedResourceTypeFlags::RenderTarget;
        };

        struct Scope
        {
            ScopeId m_scopeId;

            /// The hardware queue class that this scope executed on.
            HardwareQueueClass m_hardwareQueueClass = HardwareQueueClass::Graphics;
        };

        struct MemoryUsage
        {
            size_t m_bufferMemoryInBytes = 0;
            size_t m_imageMemoryInBytes = 0;
            size_t m_rendertargetMemoryInBytes = 0;
        };

        AllocationPolicy m_allocationPolicy = AllocationPolicy::HeapPlacement;

        /// Flat array of scopes used last frame.
        AZStd::vector<Scope> m_scopes;

        /// Flat array of heaps used last frame.
        AZStd::vector<Heap> m_heaps;

        //! Reserved memory used by the transient pool.
        MemoryUsage m_reservedMemory;
    };
}

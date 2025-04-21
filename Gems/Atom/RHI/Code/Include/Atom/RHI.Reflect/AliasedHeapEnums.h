/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! The types of resources that can be aliased.
    enum class AliasedResourceType : uint32_t
    {
        Buffer = 0,
        Image,
        RenderTarget,
        Count
    };

    //! Flags to describe the resources supported by a heap.
    enum class AliasedResourceTypeFlags : uint32_t
    {
        Buffer = AZ_BIT(static_cast<uint32_t>(AliasedResourceType::Buffer)),
        Image = AZ_BIT(static_cast<uint32_t>(AliasedResourceType::Image)),
        RenderTarget = AZ_BIT(static_cast<uint32_t>(AliasedResourceType::RenderTarget)),
        All = Buffer | Image | RenderTarget
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::AliasedResourceTypeFlags)

    //! Parameters when using the Paging heap allocation strategy.
    struct HeapPagingParameters
    {
        AZ_TYPE_INFO(HeapPagingParameters, "{530768C3-BE3B-4E8E-A6F6-1391FE813887}");
        static void Reflect(AZ::ReflectContext* context);

        static constexpr size_t DefaultPageSize = 64 * 1024 * 1024;
        static constexpr uint32_t DefaultCollectLatency = 1;
        static constexpr float DefaultInitialAllocationPercentage = 0.6f;

        //! Size of the page to allocate.
        AZ::u64 m_pageSizeInBytes = DefaultPageSize;
        //! Percentage of the budget that must be allocated at initialization.
        float m_initialAllocationPercentage = DefaultInitialAllocationPercentage;
        //! Number of frames before an empty page is release.
        uint32_t m_collectLatency = DefaultCollectLatency;
    };

    //! Parameters when using the MemoryHint heap allocation strategy.
    struct HeapMemoryHintParameters
    {
        AZ_TYPE_INFO(HeapMemoryHintParameters, "{7B448FF1-62CF-4758-9753-D2FB64E73620}");
        static void Reflect(AZ::ReflectContext* context);

        static constexpr size_t DefaultMinHeapSize = 32 * 1024 * 1024;
        static constexpr uint32_t DefaultCollectLatency = 1;
        static constexpr float DefaultScaleFactor = 1.0f;
        static constexpr float DefaultMaxWastedPercentage = 0.35f;

        //! Minimum size of a heap to use.
        AZ::u64 m_minHeapSizeInBytes = DefaultMinHeapSize;
        //! Number of frames a heap must be over the m_maxHeapWastedPercentage to compact (or release if is empty).
        uint32_t m_collectLatency = DefaultCollectLatency;
        //! Scale factor applied when allocating a new heap. Useful for allocating "extra" space to avoid new allocations during window resizing.
        float m_heapSizeScaleFactor = DefaultScaleFactor;
        //! Max percentage of wasted space of a heap before is compacted.
        float m_maxHeapWastedPercentage = DefaultMaxWastedPercentage;
    };

    enum class HeapAllocationStrategy : uint32_t
    {
        Fixed = 0,  // The whole budget is allocated at initialization. No resizing.
        Paging,     // Part of the budget is allocated at initialization. Pool grows/shrinks by allocating/deallocating pages.
        MemoryHint  // Pool grows/shrinks by allocating/deallocating heaps based on a memory usage hint that is passed.
    };

    //! Parameters that controls how to allocate resources
    //! based on heap allocation strategy picked for a transient pool.
    struct HeapAllocationParameters
    {
        HeapAllocationParameters()
            : m_pagingParameters()
        {}

        HeapAllocationParameters(const HeapMemoryHintParameters& hintParameters)
            : m_type(HeapAllocationStrategy::MemoryHint)
            , m_usageHintParameters(hintParameters)
        {
        }

        HeapAllocationParameters(const HeapPagingParameters& pagingParameters)
            : m_type(HeapAllocationStrategy::Paging)
            , m_pagingParameters(pagingParameters)
        {
        }

        HeapAllocationStrategy m_type = HeapAllocationStrategy::Fixed;
        union
        {
            HeapPagingParameters m_pagingParameters;
            HeapMemoryHintParameters m_usageHintParameters;
        };
    };

    const char* ToString(HeapAllocationStrategy type);
    const char* ToString(AliasedResourceType type);
}

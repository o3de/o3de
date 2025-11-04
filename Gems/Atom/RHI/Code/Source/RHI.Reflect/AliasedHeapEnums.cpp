/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/AliasedHeapEnums.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    const char* ToString(HeapAllocationStrategy type)
    {
        switch (type)
        {
        case HeapAllocationStrategy::Fixed: return "Fixed";
        case HeapAllocationStrategy::Paging: return "Paging";
        case HeapAllocationStrategy::MemoryHint: return "MemoryHint";
        default:
            return "Invalid";
        }
    }

    const char* ToString(AliasedResourceType type)
    {
        switch (type)
        {
        case AliasedResourceType::Buffer:       return "Buffer";
        case AliasedResourceType::Image:        return "Image";
        case AliasedResourceType::RenderTarget: return "Rendertarget";
        default:
            return "Invalid";
        }
    }

    void HeapPagingParameters::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeapPagingParameters>()
                ->Version(0)
                ->Field("m_pageSizeInBytes", &HeapPagingParameters::m_pageSizeInBytes)
                ->Field("m_initialAllocationPercentage", &HeapPagingParameters::m_initialAllocationPercentage)
                ->Field("m_collectLatency", &HeapPagingParameters::m_collectLatency)
                ;
        }
    }
        
    void HeapMemoryHintParameters::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeapMemoryHintParameters>()
                ->Version(0)
                ->Field("m_minHeapSizeInBytes", &HeapMemoryHintParameters::m_minHeapSizeInBytes)
                ->Field("m_collectLatency", &HeapMemoryHintParameters::m_collectLatency)
                ->Field("m_heapSizeScaleFactor", &HeapMemoryHintParameters::m_heapSizeScaleFactor)
                ->Field("m_maxHeapWastedPercentage", &HeapMemoryHintParameters::m_maxHeapWastedPercentage)
                ;
        }
    }
}

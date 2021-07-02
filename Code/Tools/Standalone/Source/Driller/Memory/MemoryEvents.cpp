/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "MemoryEvents.h"

#include "MemoryDataAggregator.hxx"

namespace Driller
{
    void    MemoryDrillerRegisterAllocatorEvent::StepForward(Aggregator* data)
    {
        MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
        // add to list of active allocators
        aggr->m_allocators.push_back(&m_allocatorInfo);
    }

    void    MemoryDrillerRegisterAllocatorEvent::StepBackward(Aggregator* data)
    {
        MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
        // remove from the list of active allocators
        aggr->m_allocators.erase(AZStd::find(aggr->m_allocators.begin(), aggr->m_allocators.end(), &m_allocatorInfo));
    }

    void    MemoryDrillerUnregisterAllocatorEvent::StepForward(Aggregator* data)
    {
        MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
        MemoryDataAggregator::AllocatorInfoArrayType::iterator alIt = aggr->FindAllocatorById(m_allocatorId);
        m_removedAllocatorInfo = *alIt;
        aggr->m_allocators.erase(alIt);
    }

    void    MemoryDrillerUnregisterAllocatorEvent::StepBackward(Aggregator* data)
    {
        MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
        aggr->m_allocators.push_back(m_removedAllocatorInfo);
    }

    void    MemoryDrillerRegisterAllocationEvent::StepForward(Aggregator* data)
    {
        if (m_modifiedAllocatorInfo == nullptr)
        {
            MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
            MemoryDataAggregator::AllocatorInfoArrayType::iterator infoIter = aggr->FindAllocatorByRecordsId(m_allocationInfo.m_recordsId);

            if (infoIter == aggr->GetAllocatorEnd())
            {
                AZ_Assert(false, "MemoryDriller - Invalid RecordsId");
                return;
            }

            m_modifiedAllocatorInfo = (*infoIter);
        }
        // add to map of allocations
        m_modifiedAllocatorInfo->m_allocations.insert(AZStd::make_pair(m_address, &m_allocationInfo));
        m_modifiedAllocatorInfo->m_allocatedMemory += m_allocationInfo.m_size;
    }

    void    MemoryDrillerRegisterAllocationEvent::StepBackward(Aggregator* data)
    {
        if (m_modifiedAllocatorInfo == nullptr)
        {
            MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
            MemoryDataAggregator::AllocatorInfoArrayType::iterator infoIter = aggr->FindAllocatorByRecordsId(m_allocationInfo.m_recordsId);

            if (infoIter == aggr->GetAllocatorEnd())
            {
                AZ_Assert(false, "MemoryDriller - Invalid RecordsId");
                return;
            }

            m_modifiedAllocatorInfo = (*infoIter);
        }

        // remove from the list of active allocators
        m_modifiedAllocatorInfo->m_allocations.erase(m_address);
        m_modifiedAllocatorInfo->m_allocatedMemory -= m_allocationInfo.m_size;
    }

    void    MemoryDrillerUnregisterAllocationEvent::StepForward(Aggregator* data)
    {
        if (m_modifiedAllocatorInfo == nullptr)
        {
            MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
            // removed from the map of allocations
            MemoryDataAggregator::AllocatorInfoArrayType::iterator infoIter = aggr->FindAllocatorByRecordsId(m_recordsId);

            if (infoIter == aggr->GetAllocatorEnd())
            {
                AZ_Assert(false, "MemoryDriller - Invalid RecordsId");
                return;
            }

            m_modifiedAllocatorInfo = (*infoIter);
        }

        Memory::AllocatorInfo::AllocationMapType::iterator allocIt = m_modifiedAllocatorInfo->m_allocations.find(m_address);
        m_removedAllocationInfo = allocIt->second;
        // we're UNALLOCATING, so subract:
        m_modifiedAllocatorInfo->m_allocatedMemory -= m_removedAllocationInfo->m_size;
        m_modifiedAllocatorInfo->m_allocations.erase(m_address);
    }

    void    MemoryDrillerUnregisterAllocationEvent::StepBackward(Aggregator* data)
    {
        if (m_modifiedAllocatorInfo == nullptr)
        {
            MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
            // removed from the map of allocations
            MemoryDataAggregator::AllocatorInfoArrayType::iterator infoIter = aggr->FindAllocatorByRecordsId(m_recordsId);

            if (infoIter == aggr->GetAllocatorEnd())
            {
                AZ_Assert(false, "MemoryDriller - Invalid RecordsId");
                return;
            }

            m_modifiedAllocatorInfo = (*infoIter);
        }

        // add back to the map of allocations
        auto insertionPair = AZStd::make_pair(m_address, m_removedAllocationInfo);
        m_modifiedAllocatorInfo->m_allocations.insert(insertionPair);

        // we're doing the opposite of unallocating, which is allocating, so we add:
        m_modifiedAllocatorInfo->m_allocatedMemory += m_removedAllocationInfo->m_size;
    }

    void    MemoryDrillerResizeAllocationEvent::StepForward(Aggregator* data)
    {
        if (m_modifiedAllocationInfo == nullptr)
        {
            MemoryDataAggregator* aggr = static_cast<MemoryDataAggregator*>(data);
            // change the allocation size
            m_modifiedAllocatorInfo = *aggr->FindAllocatorByRecordsId(m_recordsId);
            Memory::AllocatorInfo::AllocationMapType::iterator allocIt = m_modifiedAllocatorInfo->m_allocations.find(m_address);
            m_modifiedAllocationInfo = allocIt->second;
        }

        // reallocating remove old size and add new size:
        m_oldSize = m_modifiedAllocationInfo->m_size;
        m_modifiedAllocationInfo->m_size = m_newSize;

        m_modifiedAllocatorInfo->m_allocatedMemory -= m_oldSize;
        m_modifiedAllocatorInfo->m_allocatedMemory += m_newSize;
    }

    void    MemoryDrillerResizeAllocationEvent::StepBackward(Aggregator* data)
    {
        (void)data;
        // restore the old size
        m_modifiedAllocationInfo->m_size = m_oldSize;

        m_modifiedAllocatorInfo->m_allocatedMemory -= m_newSize;
        m_modifiedAllocatorInfo->m_allocatedMemory += m_oldSize;
    }
} // namespace Driller

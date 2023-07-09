/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/QueryPoolSubAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    void QueryPoolSubAllocator::Init(uint32_t maxQueryCount)
    {
        m_freeAllocations.push_back(Allocation(0, maxQueryCount));
        m_totalFreeSpace = maxQueryCount;
    }

    AZStd::vector<QueryPoolSubAllocator::Allocation> QueryPoolSubAllocator::Allocate(uint32_t count)
    {
        AZStd::vector<QueryPoolSubAllocator::Allocation> allocations;
        if (m_totalFreeSpace < count)
        {
            return allocations;
        }
            
        uint32_t remainingCount = count;
        // Find a free space that fits the best for the amount that needs to be allocated.
        auto findIt = AZStd::lower_bound(m_freeAllocations.begin(), m_freeAllocations.end(), Allocation(0, count), SortBySize());
        if (findIt == m_freeAllocations.end())
        {
            // No enough space, need to allocate in pieces
            AZStd::vector<Allocation>::reverse_iterator it;
            // Let the last allocation be handled later because it may have some remaining space.
            for (it = m_freeAllocations.rbegin(); it != m_freeAllocations.rend() && remainingCount > (*it).m_count; ++it)
            {
                Allocation& freeAlloc = *it;
                Allocation newAlloc(freeAlloc.m_offset, freeAlloc.m_count);
                allocations.push_back(newAlloc);
                remainingCount -= freeAlloc.m_count;
                m_freeAllocations.erase(AZStd::next(it).base());
            }

            findIt = AZStd::next(it).base();
        }

        Allocation& freeAlloc = *findIt;
        Allocation newAlloc(freeAlloc.m_offset, remainingCount);
        allocations.push_back(newAlloc);
        uint32_t remainingFreeCount = freeAlloc.m_count - newAlloc.m_count;
        if (remainingFreeCount)
        {
            // Resize the free allocation.
            freeAlloc.m_offset += newAlloc.m_count;
            freeAlloc.m_count = remainingFreeCount;
            // Need to sort because we changed the size.
            SortFreeSpaces();
        }
        else
        {
            // Just remove it because we use the full free space.
            m_freeAllocations.erase(findIt);
        }

        AZStd::copy(allocations.begin(), allocations.end(), AZStd::inserter(m_allocations, m_allocations.end()));
        m_totalFreeSpace -= count;
        return allocations;
    }

    void QueryPoolSubAllocator::DeAllocate(uint32_t offset)
    {
        // Find the allocation that contains the offset.
        auto findIt = m_allocations.upper_bound(Allocation(offset, 0));
        --findIt;

        Allocation allocation = *findIt;
        AZ_Assert(offset >= allocation.m_offset && offset < (allocation.m_offset + allocation.m_count), "Invalid deallocation %d", offset);
        m_allocations.erase(findIt);
        // Split the allocation in two, the left and the right side (if needed).
        if (offset > allocation.m_offset)
        {
            Allocation newAllocation (allocation.m_offset, offset - allocation.m_offset);
            m_allocations.insert(newAllocation);
        }

        uint32_t allocationEnd = allocation.m_offset + allocation.m_count - 1;
        if (allocationEnd > offset)
        {
            Allocation newAllocation(offset + 1, allocationEnd - offset);
            m_allocations.insert(newAllocation);
        }

        // Add the free space.
        Allocation freeSpace(offset , 1);
        AddFreeSpace(freeSpace);
    }

    void QueryPoolSubAllocator::AddFreeSpace(const Allocation& allocation)
    {
        // First check if we need to merge the free space.
        auto mergeLeft = m_freeAllocations.end();
        auto mergeRight = m_freeAllocations.end();
        for (auto it = m_freeAllocations.begin(); 
            it != m_freeAllocations.end() && (mergeLeft == m_freeAllocations.end() || mergeRight == m_freeAllocations.end());
            ++it)
        {
            Allocation& freeAllocation = *it;
            // Check if the allocation is contiguous on the left side.
            if (mergeLeft == m_freeAllocations.end() && 
                (freeAllocation.m_offset + freeAllocation.m_count) == allocation.m_offset)
            {
                // Add the new free space to the old allocation.
                freeAllocation.m_count += allocation.m_count;
                mergeLeft = it;
            }

            // Check if the allocation is contiguous on the right side.
            if (mergeRight == m_freeAllocations.end() && 
                (allocation.m_offset + allocation.m_count) == freeAllocation.m_offset)
            {
                // Add the new free space to the old allocation and adjust the offset.
                freeAllocation.m_offset = allocation.m_offset;
                freeAllocation.m_count += allocation.m_count;
                mergeRight = it;
            }
        }

        bool didMergeLeft = mergeLeft != m_freeAllocations.end();
        bool didMergeRight = mergeRight != m_freeAllocations.end();
        if (!didMergeLeft && !didMergeRight)
        {
            // No merge happened so we just insert the new free space.
            m_freeAllocations.push_back(allocation);
        }
        else if (didMergeLeft && didMergeRight)
        {
            // We merged from both sides so we need to merge both free spaces (left and right).
            // The new free space size was already included in mergeRight->m_size so we need to substract it
            // to prevent adding it twice.
            mergeLeft->m_count += mergeRight->m_count - allocation.m_count;
            m_freeAllocations.erase(mergeRight);
        }

        SortFreeSpaces();
        m_totalFreeSpace += allocation.m_count;
    }

    void QueryPoolSubAllocator::SortFreeSpaces()
    {
        AZStd::sort(m_freeAllocations.begin(), m_freeAllocations.end(), SortBySize());
    }
}

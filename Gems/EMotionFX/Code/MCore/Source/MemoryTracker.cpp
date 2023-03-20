/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MemoryTracker.h"
#include "LogManager.h"


namespace MCore
{
    // global stats constructor
    MemoryTracker::GlobalStats::GlobalStats()
    {
        m_currentNumBytes    = 0;
        m_currentNumAllocs   = 0;
        m_totalNumAllocs     = 0;
        m_totalNumReallocs   = 0;
        m_totalNumFrees      = 0;
    }


    // category stats constructor
    MemoryTracker::CategoryStats::CategoryStats()
    {
        m_currentNumBytes    = 0;
        m_currentNumAllocs   = 0;
        m_totalNumAllocs     = 0;
        m_totalNumReallocs   = 0;
        m_totalNumFrees      = 0;
    }


    // group statistics
    MemoryTracker::GroupStats::GroupStats()
    {
        m_currentNumBytes    = 0;
        m_currentNumAllocs   = 0;
        m_totalNumAllocs     = 0;
        m_totalNumReallocs   = 0;
        m_totalNumFrees      = 0;
    }


    //-------------------------------------------------------------

    // constructor
    MemoryTracker::MemoryTracker()
    {
    }


    // destructor
    MemoryTracker::~MemoryTracker()
    {
        Clear();
    }


    void MemoryTracker::RegisterAlloc(void* memAddress, size_t numBytes, uint32 categoryID)
    {
        LockGuard lock(m_mutex);
        RegisterAllocNoLock(memAddress, numBytes, categoryID);
    }


    // register an allocation
    void MemoryTracker::RegisterAllocNoLock(void* memAddress, size_t numBytes, uint32 categoryID)
    {
        // insert the new allocation
        Allocation newAlloc;
        newAlloc.m_memAddress    = memAddress;
        newAlloc.m_numBytes      = numBytes;
        newAlloc.m_categoryId    = categoryID;
        m_allocs.insert(std::make_pair(memAddress, newAlloc));

        // update global stats
        m_globalStats.m_totalNumAllocs++;
        m_globalStats.m_currentNumAllocs++;
        m_globalStats.m_currentNumBytes += numBytes;

        // update the category stats
        auto categoryItem = m_categories.find(categoryID);
        if (categoryItem != m_categories.end())
        {
            CategoryStats& catStats = categoryItem->second;
            catStats.m_totalNumAllocs++;
            catStats.m_currentNumAllocs++;
            catStats.m_currentNumBytes += numBytes;
        }
        else
        {
            // auto register the new category
            CategoryStats catStats;
            catStats.m_totalNumAllocs++;
            catStats.m_currentNumAllocs++;
            catStats.m_currentNumBytes += numBytes;
            m_categories.insert(std::make_pair(categoryID, catStats));
        }
    }


    void MemoryTracker::RegisterRealloc(void* oldAddress, void* newAddress, size_t numBytes, uint32 categoryID)
    {
        LockGuard lock(m_mutex);
        RegisterReallocNoLock(oldAddress, newAddress, numBytes, categoryID);
    }


    // register a reallocation
    void MemoryTracker::RegisterReallocNoLock(void* oldAddress, void* newAddress, size_t numBytes, uint32 categoryID)
    {
        // if we basically just allocate, use that instead
        if (oldAddress == nullptr)
        {
            RegisterAllocNoLock(newAddress, numBytes, categoryID);
            return;
        }

        // try to locate the allocation
        auto item = m_allocs.find(oldAddress);

        // if we found the item
        if (item != m_allocs.end())
        {
            const Allocation& allocation = item->second;
            const size_t oldNumBytes = allocation.m_numBytes;

            // update the global stats
            m_globalStats.m_currentNumBytes += (numBytes - oldNumBytes);
            m_globalStats.m_totalNumReallocs++;

            // the category got updated, unregister it from the old category
            const bool categoryChanged = (categoryID != allocation.m_categoryId);
            if (categoryChanged)
            {
                auto oldCategoryItem = m_categories.find(allocation.m_categoryId);
                MCORE_ASSERT(oldCategoryItem != m_categories.end());
                oldCategoryItem->second.m_currentNumBytes -= oldNumBytes;
                oldCategoryItem->second.m_currentNumAllocs--;
            }

            // remove the allocation
            m_allocs.erase(item);

            // re-insert it using the new address (new key)
            Allocation newAlloc;
            newAlloc.m_categoryId    = categoryID;
            newAlloc.m_memAddress    = newAddress;
            newAlloc.m_numBytes      = numBytes;
            m_allocs.insert(std::make_pair(newAddress, newAlloc));

            // update the category stats
            auto categoryItem = m_categories.find(categoryID);
            MCORE_ASSERT(categoryItem != m_categories.end());

            CategoryStats& catStats = categoryItem->second;
            catStats.m_totalNumReallocs++;
            if (categoryChanged == false)
            {
                catStats.m_currentNumBytes += (numBytes - oldNumBytes);
            }
            else
            {
                catStats.m_currentNumBytes += numBytes;
                catStats.m_currentNumAllocs++;
            }
        }
        else // not found, we must just register this as a regular allocation
        {
            RegisterAllocNoLock(newAddress, numBytes, categoryID);
        }
    }


    void MemoryTracker::RegisterFree(void* memAddress)
    {
        LockGuard lock(m_mutex);
        RegisterFreeNoLock(memAddress);
    }


    // register a free
    void MemoryTracker::RegisterFreeNoLock(void* memAddress)
    {
        if (memAddress == nullptr)
        {
            return;
        }

        // find the allocation using this address
        auto item = m_allocs.find(memAddress);
        if (item != m_allocs.end())
        {
            Allocation& allocation = item->second;

            // update global stats
            m_globalStats.m_currentNumBytes -= allocation.m_numBytes;
            m_globalStats.m_totalNumFrees++;
            m_globalStats.m_currentNumAllocs--;

            // update the category stats
            auto categoryItem = m_categories.find(allocation.m_categoryId);
            MCORE_ASSERT(categoryItem != m_categories.end()); // this should be impossible

            CategoryStats& catStats = categoryItem->second;
            catStats.m_currentNumAllocs--;
            catStats.m_totalNumFrees++;
            catStats.m_currentNumBytes -= allocation.m_numBytes;

            // remove the allocation
            m_allocs.erase(item);
        }
        else
        {
            // this should never happen, we are trying to free an address that has not been
            // allocated before or has already been freed
            MCORE_ASSERT(false);
        }
    }


    // clear all stored allocations
    void MemoryTracker::Clear()
    {
        LockGuard lock(m_mutex);
        m_allocs.clear();
        m_categories.clear();
        m_groups.clear();
        m_globalStats = GlobalStats();
    }


    // get the global stats
    const MemoryTracker::GlobalStats& MemoryTracker::GetGlobalStats() const
    {
        return m_globalStats;
    }


    // get category statistics
    bool MemoryTracker::GetCategoryStatistics(uint32 categoryID, CategoryStats* outStats)
    {
        LockGuard lock(m_mutex);
        return GetCategoryStatisticsNoLock(categoryID, outStats);
    }


    // get category statistics, without lock
    bool MemoryTracker::GetCategoryStatisticsNoLock(uint32 categoryID, CategoryStats* outStats)
    {
        auto item = m_categories.find(categoryID);
        if (item != m_categories.end())
        {
            *outStats = item->second;
            return true;
        }

        return false;
    }


    // register the name to a given category
    void MemoryTracker::RegisterCategory(uint32 categoryID, const char* name)
    {
        LockGuard lock(m_mutex);

        // try to see if the category exists already
        auto item = m_categories.find(categoryID);
        if (item == m_categories.end())
        {
            // register the new category
            CategoryStats catStats;
            catStats.m_name = name;
            m_categories.insert(std::make_pair(categoryID, catStats));
        }
        else
        {
            item->second.m_name = name;
        }
    }


    // register a given group
    void MemoryTracker::RegisterGroup(uint32 groupID, const char* name, const std::vector<uint32>& categories)
    {
        LockGuard lock(m_mutex);

        auto item = m_groups.find(groupID);
        if (item == m_groups.end())
        {
            // create a new group
            Group newGroup;
            newGroup.m_name = name;
            for (auto& cat : categories)
            {
                newGroup.m_categories.insert(cat);
            }
            m_groups.insert(std::make_pair(groupID, newGroup));
        }
        else
        {
            // update the existing group by inserting categories that don't exist yet inside the group
            Group& group = item->second;
            group.m_name = name;
            for (auto& cat : categories)
            {
                if (group.m_categories.find(cat) == group.m_categories.end())
                {
                    group.m_categories.insert(cat);
                }
            }
        }
    }


    // update the statistics for all groups
    void MemoryTracker::UpdateGroupStatistics()
    {
        LockGuard lock(m_mutex);
        UpdateGroupStatisticsNoLock();
    }


    //
    void MemoryTracker::UpdateGroupStatisticsNoLock()
    {
        // for all registered groups
        for (auto& item : m_groups)
        {
            Group& group = item.second;

            // reset the totals
            group.m_stats = GroupStats();

            // for all categories
            for (auto categoryID : group.m_categories)
            {
                CategoryStats categoryStats;
                GetCategoryStatisticsNoLock(categoryID, &categoryStats);

                group.m_stats.m_currentNumAllocs  += categoryStats.m_currentNumAllocs;
                group.m_stats.m_currentNumBytes   += categoryStats.m_currentNumBytes;
                group.m_stats.m_totalNumAllocs    += categoryStats.m_totalNumAllocs;
                group.m_stats.m_totalNumFrees     += categoryStats.m_totalNumFrees;
                group.m_stats.m_totalNumReallocs  += categoryStats.m_totalNumReallocs;
            }
        }
    }

    // get group statistics
    bool MemoryTracker::GetGroupStatistics(uint32 groupID, GroupStats* outGroupStats)
    {
        LockGuard lock(m_mutex);

        auto item = m_groups.find(groupID);
        if (item == m_groups.end())
        {
            return false;
        }

        const GroupStats& groupStats = item->second.m_stats;
        *outGroupStats = groupStats;

        return true;
    }


    // get the allocations
    const std::unordered_map<void*, MemoryTracker::Allocation>& MemoryTracker::GetAllocations() const
    {
        return m_allocs;
    }


    // get the groups
    const std::unordered_map<uint32, MemoryTracker::Group>& MemoryTracker::GetGroups() const
    {
        return m_groups;
    }


    // get the categories
    const std::map<uint32, MemoryTracker::CategoryStats>& MemoryTracker::GetCategories() const
    {
        return m_categories;
    }


    // multithread lock
    void MemoryTracker::Lock()
    {
        m_mutex.Lock();
    }


    // multithread unlock
    void MemoryTracker::Unlock()
    {
        m_mutex.Unlock();
    }


    // log the stats
    void MemoryTracker::LogStatistics(bool currentlyAllocatedOnly)
    {
        LockGuard lock(m_mutex);

        // update the group statistics (thread safe also)
        UpdateGroupStatisticsNoLock();

        Print("--[ Memory Global Statistics ]-----------------------------------------------------------------------");
        Print(FormatStdString("Current Num Bytes Used = %d bytes (%d k or %.2f mb)", m_globalStats.m_currentNumBytes, m_globalStats.m_currentNumBytes / 1000, m_globalStats.m_currentNumBytes / 1000000.0f).c_str());
        Print(FormatStdString("Current Num Allocs     = %d", m_globalStats.m_currentNumAllocs).c_str());
        Print(FormatStdString("Total Num Allocs       = %d", m_globalStats.m_totalNumAllocs).c_str());
        Print(FormatStdString("Total Num Reallocs     = %d", m_globalStats.m_totalNumReallocs).c_str());
        Print(FormatStdString("Total Num Frees        = %d", m_globalStats.m_totalNumFrees).c_str());

        if (m_categories.empty() == false)
        {
            Print("");
            Print("--[ Memory Category Statistics ]---------------------------------------------------------------------");
            for (auto& item : m_categories)
            {
                const uint32            categoryID  = item.first;
                const CategoryStats&    stats       = item.second;
                if (stats.m_totalNumAllocs > 0)
                {
                    bool display = true;
                    if (currentlyAllocatedOnly)
                    {
                        if (stats.m_currentNumAllocs == 0)
                        {
                            display = false;
                        }
                    }

                    if (display)
                    {
                        Print(FormatStdString("[Cat %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", categoryID, stats.m_currentNumBytes, stats.m_currentNumBytes / 1000, stats.m_currentNumAllocs, stats.m_totalNumAllocs, stats.m_totalNumReallocs, stats.m_totalNumFrees, stats.m_name.c_str()).c_str());
                    }
                }
            }
        }

        if (m_groups.empty() == false)
        {
            Print("");
            Print("--[ Group Statistics ]-------------------------------------------------------------------------------");
            for (auto& item : m_groups)
            {
                const uint32 groupID    = item.first;
                const Group& group      = item.second;
                if (group.m_stats.m_totalNumAllocs > 0)
                {
                    bool display = true;
                    if (currentlyAllocatedOnly)
                    {
                        if (group.m_stats.m_currentNumAllocs == 0)
                        {
                            display = false;
                        }
                    }

                    if (display)
                    {
                        Print(FormatStdString("[Group %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", groupID, group.m_stats.m_currentNumBytes, group.m_stats.m_currentNumBytes / 1000, group.m_stats.m_currentNumAllocs, group.m_stats.m_totalNumAllocs, group.m_stats.m_totalNumReallocs, group.m_stats.m_totalNumFrees, group.m_name.c_str()).c_str());
                    }
                }
            }
        }
    }


    // log the stats
    void MemoryTracker::LogLeaks()
    {
        LockGuard lock(m_mutex);

        // update the group statistics (thread safe also)
        UpdateGroupStatisticsNoLock();

        if (m_allocs.size() == 0)
        {
            Print("MCore::MemoryTracker::LogLeaks() - No memory leaks have been detected.");
            return;
        }

        // log globals
        Print("--[ Memory Leak Global Statistics ]-----------------------------------------------------------------------");
        Print(FormatStdString("Leaking Num Bytes   = %d bytes (%d k or %.2f mb)", m_globalStats.m_currentNumBytes, m_globalStats.m_currentNumBytes / 1000, m_globalStats.m_currentNumBytes / 1000000.0f).c_str());
        Print(FormatStdString("Leaking Num Allocs  = %d", m_globalStats.m_currentNumAllocs).c_str());
        Print("");

        // log category totals
        Print("--[ Memory Category Leak Statistics ]---------------------------------------------------------------------");
        for (auto& item : m_categories)
        {
            const uint32            categoryID  = item.first;
            const CategoryStats&    stats       = item.second;
            if (stats.m_currentNumAllocs > 0)
            {
                Print(FormatStdString("[Cat %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", categoryID, stats.m_currentNumBytes, stats.m_currentNumBytes / 1000, stats.m_currentNumAllocs, stats.m_totalNumAllocs, stats.m_totalNumReallocs, stats.m_totalNumFrees, stats.m_name.c_str()).c_str());
            }
        }
        Print("");

        if (m_groups.empty() == false)
        {
            Print("");
            Print("--[ Group Statistics ]-------------------------------------------------------------------------------");
            for (auto& item : m_groups)
            {
                const uint32 groupID    = item.first;
                const Group& group      = item.second;
                if (group.m_stats.m_currentNumAllocs > 0)
                {
                    Print(FormatStdString("[Group %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", groupID, group.m_stats.m_currentNumBytes, group.m_stats.m_currentNumBytes / 1000, group.m_stats.m_currentNumAllocs, group.m_stats.m_totalNumAllocs, group.m_stats.m_totalNumReallocs, group.m_stats.m_totalNumFrees, group.m_name.c_str()).c_str());
                }
            }
            Print("");
        }

        // log individual leaks
        Print("--[ Memory Allocations ]----------------------------------------------------------------------------------");
        uint32 allocNumber = 0;
        for (auto& item : m_allocs)
        {
            const char*         data        = static_cast<char*>(item.first);
            const Allocation&   allocation  = item.second;

            char buffer[64];
            memset(buffer, 0, 64);

            const size_t numBytes = (allocation.m_numBytes >= 64) ? 63 : allocation.m_numBytes;
            for (uint32 i = 0; i < numBytes; ++i)
            {
                char c = data[i];
                if (c < 32 || c >= 126)
                {
                    c = '.';
                }
                buffer[i] = c;
            }

            auto catItem = m_categories.find(allocation.m_categoryId);
            MCORE_ASSERT(catItem != m_categories.end());
            Print(FormatStdString("#%-4d - %6d bytes (cat=%4d) - [%-66s] --> %s", allocNumber, allocation.m_numBytes, allocation.m_categoryId, buffer, catItem->second.m_name.c_str()).c_str());

            allocNumber++;
        }
    }
}   // namespace MCore

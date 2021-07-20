/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mCurrentNumBytes    = 0;
        mCurrentNumAllocs   = 0;
        mTotalNumAllocs     = 0;
        mTotalNumReallocs   = 0;
        mTotalNumFrees      = 0;
    }


    // category stats constructor
    MemoryTracker::CategoryStats::CategoryStats()
    {
        mCurrentNumBytes    = 0;
        mCurrentNumAllocs   = 0;
        mTotalNumAllocs     = 0;
        mTotalNumReallocs   = 0;
        mTotalNumFrees      = 0;
    }


    // group statistics
    MemoryTracker::GroupStats::GroupStats()
    {
        mCurrentNumBytes    = 0;
        mCurrentNumAllocs   = 0;
        mTotalNumAllocs     = 0;
        mTotalNumReallocs   = 0;
        mTotalNumFrees      = 0;
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
        LockGuard lock(mMutex);
        RegisterAllocNoLock(memAddress, numBytes, categoryID);
    }


    // register an allocation
    void MemoryTracker::RegisterAllocNoLock(void* memAddress, size_t numBytes, uint32 categoryID)
    {
        // insert the new allocation
        Allocation newAlloc;
        newAlloc.mMemAddress    = memAddress;
        newAlloc.mNumBytes      = numBytes;
        newAlloc.mCategoryID    = categoryID;
        mAllocs.insert(std::make_pair(memAddress, newAlloc));

        // update global stats
        mGlobalStats.mTotalNumAllocs++;
        mGlobalStats.mCurrentNumAllocs++;
        mGlobalStats.mCurrentNumBytes += numBytes;

        // update the category stats
        auto categoryItem = mCategories.find(categoryID);
        if (categoryItem != mCategories.end())
        {
            CategoryStats& catStats = categoryItem->second;
            catStats.mTotalNumAllocs++;
            catStats.mCurrentNumAllocs++;
            catStats.mCurrentNumBytes += numBytes;
        }
        else
        {
            // auto register the new category
            CategoryStats catStats;
            catStats.mTotalNumAllocs++;
            catStats.mCurrentNumAllocs++;
            catStats.mCurrentNumBytes += numBytes;
            mCategories.insert(std::make_pair(categoryID, catStats));
        }
    }


    void MemoryTracker::RegisterRealloc(void* oldAddress, void* newAddress, size_t numBytes, uint32 categoryID)
    {
        LockGuard lock(mMutex);
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
        auto item = mAllocs.find(oldAddress);

        // if we found the item
        if (item != mAllocs.end())
        {
            const Allocation& allocation = item->second;
            const size_t oldNumBytes = allocation.mNumBytes;

            // update the global stats
            mGlobalStats.mCurrentNumBytes += (numBytes - oldNumBytes);
            mGlobalStats.mTotalNumReallocs++;

            // the category got updated, unregister it from the old category
            const bool categoryChanged = (categoryID != allocation.mCategoryID);
            if (categoryChanged)
            {
                auto oldCategoryItem = mCategories.find(allocation.mCategoryID);
                MCORE_ASSERT(oldCategoryItem != mCategories.end());
                oldCategoryItem->second.mCurrentNumBytes -= oldNumBytes;
                oldCategoryItem->second.mCurrentNumAllocs--;
            }

            // remove the allocation
            mAllocs.erase(item);

            // re-insert it using the new address (new key)
            Allocation newAlloc;
            newAlloc.mCategoryID    = categoryID;
            newAlloc.mMemAddress    = newAddress;
            newAlloc.mNumBytes      = numBytes;
            mAllocs.insert(std::make_pair(newAddress, newAlloc));

            // update the category stats
            auto categoryItem = mCategories.find(categoryID);
            MCORE_ASSERT(categoryItem != mCategories.end());

            CategoryStats& catStats = categoryItem->second;
            catStats.mTotalNumReallocs++;
            if (categoryChanged == false)
            {
                catStats.mCurrentNumBytes += (numBytes - oldNumBytes);
            }
            else
            {
                catStats.mCurrentNumBytes += numBytes;
                catStats.mCurrentNumAllocs++;
            }
        }
        else // not found, we must just register this as a regular allocation
        {
            RegisterAllocNoLock(newAddress, numBytes, categoryID);
        }
    }


    void MemoryTracker::RegisterFree(void* memAddress)
    {
        LockGuard lock(mMutex);
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
        auto item = mAllocs.find(memAddress);
        if (item != mAllocs.end())
        {
            Allocation& allocation = item->second;

            // update global stats
            mGlobalStats.mCurrentNumBytes -= allocation.mNumBytes;
            mGlobalStats.mTotalNumFrees++;
            mGlobalStats.mCurrentNumAllocs--;

            // update the category stats
            auto categoryItem = mCategories.find(allocation.mCategoryID);
            MCORE_ASSERT(categoryItem != mCategories.end()); // this should be impossible

            CategoryStats& catStats = categoryItem->second;
            catStats.mCurrentNumAllocs--;
            catStats.mTotalNumFrees++;
            catStats.mCurrentNumBytes -= allocation.mNumBytes;

            // remove the allocation
            mAllocs.erase(item);
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
        LockGuard lock(mMutex);
        mAllocs.clear();
        mCategories.clear();
        mGroups.clear();
        mGlobalStats = GlobalStats();
    }


    // get the global stats
    const MemoryTracker::GlobalStats& MemoryTracker::GetGlobalStats() const
    {
        return mGlobalStats;
    }


    // get category statistics
    bool MemoryTracker::GetCategoryStatistics(uint32 categoryID, CategoryStats* outStats)
    {
        LockGuard lock(mMutex);
        return GetCategoryStatisticsNoLock(categoryID, outStats);
    }


    // get category statistics, without lock
    bool MemoryTracker::GetCategoryStatisticsNoLock(uint32 categoryID, CategoryStats* outStats)
    {
        auto item = mCategories.find(categoryID);
        if (item != mCategories.end())
        {
            *outStats = item->second;
            return true;
        }

        return false;
    }


    // register the name to a given category
    void MemoryTracker::RegisterCategory(uint32 categoryID, const char* name)
    {
        LockGuard lock(mMutex);

        // try to see if the category exists already
        auto item = mCategories.find(categoryID);
        if (item == mCategories.end())
        {
            // register the new category
            CategoryStats catStats;
            catStats.mName = name;
            mCategories.insert(std::make_pair(categoryID, catStats));
        }
        else
        {
            item->second.mName = name;
        }
    }


    // register a given group
    void MemoryTracker::RegisterGroup(uint32 groupID, const char* name, const std::vector<uint32>& categories)
    {
        LockGuard lock(mMutex);

        auto item = mGroups.find(groupID);
        if (item == mGroups.end())
        {
            // create a new group
            Group newGroup;
            newGroup.mName = name;
            for (auto& cat : categories)
            {
                newGroup.mCategories.insert(cat);
            }
            mGroups.insert(std::make_pair(groupID, newGroup));
        }
        else
        {
            // update the existing group by inserting categories that don't exist yet inside the group
            Group& group = item->second;
            group.mName = name;
            for (auto& cat : categories)
            {
                if (group.mCategories.find(cat) == group.mCategories.end())
                {
                    group.mCategories.insert(cat);
                }
            }
        }
    }


    // update the statistics for all groups
    void MemoryTracker::UpdateGroupStatistics()
    {
        LockGuard lock(mMutex);
        UpdateGroupStatisticsNoLock();
    }


    //
    void MemoryTracker::UpdateGroupStatisticsNoLock()
    {
        // for all registered groups
        for (auto& item : mGroups)
        {
            Group& group = item.second;

            // reset the totals
            group.mStats = GroupStats();

            // for all categories
            for (auto categoryID : group.mCategories)
            {
                CategoryStats categoryStats;
                GetCategoryStatisticsNoLock(categoryID, &categoryStats);

                group.mStats.mCurrentNumAllocs  += categoryStats.mCurrentNumAllocs;
                group.mStats.mCurrentNumBytes   += categoryStats.mCurrentNumBytes;
                group.mStats.mTotalNumAllocs    += categoryStats.mTotalNumAllocs;
                group.mStats.mTotalNumFrees     += categoryStats.mTotalNumFrees;
                group.mStats.mTotalNumReallocs  += categoryStats.mTotalNumReallocs;
            }
        }
    }

    // get group statistics
    bool MemoryTracker::GetGroupStatistics(uint32 groupID, GroupStats* outGroupStats)
    {
        LockGuard lock(mMutex);

        auto item = mGroups.find(groupID);
        if (item == mGroups.end())
        {
            return false;
        }

        const GroupStats& groupStats = item->second.mStats;
        *outGroupStats = groupStats;

        return true;
    }


    // get the allocations
    const std::unordered_map<void*, MemoryTracker::Allocation>& MemoryTracker::GetAllocations() const
    {
        return mAllocs;
    }


    // get the groups
    const std::unordered_map<uint32, MemoryTracker::Group>& MemoryTracker::GetGroups() const
    {
        return mGroups;
    }


    // get the categories
    const std::map<uint32, MemoryTracker::CategoryStats>& MemoryTracker::GetCategories() const
    {
        return mCategories;
    }


    // multithread lock
    void MemoryTracker::Lock()
    {
        mMutex.Lock();
    }


    // multithread unlock
    void MemoryTracker::Unlock()
    {
        mMutex.Unlock();
    }


    // log the stats
    void MemoryTracker::LogStatistics(bool currentlyAllocatedOnly)
    {
        LockGuard lock(mMutex);

        // update the group statistics (thread safe also)
        UpdateGroupStatisticsNoLock();

        Print("--[ Memory Global Statistics ]-----------------------------------------------------------------------");
        Print(FormatStdString("Current Num Bytes Used = %d bytes (%d k or %.2f mb)", mGlobalStats.mCurrentNumBytes, mGlobalStats.mCurrentNumBytes / 1000, mGlobalStats.mCurrentNumBytes / 1000000.0f).c_str());
        Print(FormatStdString("Current Num Allocs     = %d", mGlobalStats.mCurrentNumAllocs).c_str());
        Print(FormatStdString("Total Num Allocs       = %d", mGlobalStats.mTotalNumAllocs).c_str());
        Print(FormatStdString("Total Num Reallocs     = %d", mGlobalStats.mTotalNumReallocs).c_str());
        Print(FormatStdString("Total Num Frees        = %d", mGlobalStats.mTotalNumFrees).c_str());

        if (mCategories.empty() == false)
        {
            Print("");
            Print("--[ Memory Category Statistics ]---------------------------------------------------------------------");
            for (auto& item : mCategories)
            {
                const uint32            categoryID  = item.first;
                const CategoryStats&    stats       = item.second;
                if (stats.mTotalNumAllocs > 0)
                {
                    bool display = true;
                    if (currentlyAllocatedOnly)
                    {
                        if (stats.mCurrentNumAllocs == 0)
                        {
                            display = false;
                        }
                    }

                    if (display)
                    {
                        Print(FormatStdString("[Cat %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", categoryID, stats.mCurrentNumBytes, stats.mCurrentNumBytes / 1000, stats.mCurrentNumAllocs, stats.mTotalNumAllocs, stats.mTotalNumReallocs, stats.mTotalNumFrees, stats.mName.c_str()).c_str());
                    }
                }
            }
        }

        if (mGroups.empty() == false)
        {
            Print("");
            Print("--[ Group Statistics ]-------------------------------------------------------------------------------");
            for (auto& item : mGroups)
            {
                const uint32 groupID    = item.first;
                const Group& group      = item.second;
                if (group.mStats.mTotalNumAllocs > 0)
                {
                    bool display = true;
                    if (currentlyAllocatedOnly)
                    {
                        if (group.mStats.mCurrentNumAllocs == 0)
                        {
                            display = false;
                        }
                    }

                    if (display)
                    {
                        Print(FormatStdString("[Group %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", groupID, group.mStats.mCurrentNumBytes, group.mStats.mCurrentNumBytes / 1000, group.mStats.mCurrentNumAllocs, group.mStats.mTotalNumAllocs, group.mStats.mTotalNumReallocs, group.mStats.mTotalNumFrees, group.mName.c_str()).c_str());
                    }
                }
            }
        }
    }


    // log the stats
    void MemoryTracker::LogLeaks()
    {
        LockGuard lock(mMutex);

        // update the group statistics (thread safe also)
        UpdateGroupStatisticsNoLock();

        if (mAllocs.size() == 0)
        {
            Print("MCore::MemoryTracker::LogLeaks() - No memory leaks have been detected.");
            return;
        }

        // log globals
        Print("--[ Memory Leak Global Statistics ]-----------------------------------------------------------------------");
        Print(FormatStdString("Leaking Num Bytes   = %d bytes (%d k or %.2f mb)", mGlobalStats.mCurrentNumBytes, mGlobalStats.mCurrentNumBytes / 1000, mGlobalStats.mCurrentNumBytes / 1000000.0f).c_str());
        Print(FormatStdString("Leaking Num Allocs  = %d", mGlobalStats.mCurrentNumAllocs).c_str());
        Print("");

        // log category totals
        Print("--[ Memory Category Leak Statistics ]---------------------------------------------------------------------");
        for (auto& item : mCategories)
        {
            const uint32            categoryID  = item.first;
            const CategoryStats&    stats       = item.second;
            if (stats.mCurrentNumAllocs > 0)
            {
                Print(FormatStdString("[Cat %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", categoryID, stats.mCurrentNumBytes, stats.mCurrentNumBytes / 1000, stats.mCurrentNumAllocs, stats.mTotalNumAllocs, stats.mTotalNumReallocs, stats.mTotalNumFrees, stats.mName.c_str()).c_str());
            }
        }
        Print("");

        if (mGroups.empty() == false)
        {
            Print("");
            Print("--[ Group Statistics ]-------------------------------------------------------------------------------");
            for (auto& item : mGroups)
            {
                const uint32 groupID    = item.first;
                const Group& group      = item.second;
                if (group.mStats.mCurrentNumAllocs > 0)
                {
                    Print(FormatStdString("[Group %4d] - %8d bytes (%6d k) in %5d allocs [%6d / %6d / %6d] --> %s", groupID, group.mStats.mCurrentNumBytes, group.mStats.mCurrentNumBytes / 1000, group.mStats.mCurrentNumAllocs, group.mStats.mTotalNumAllocs, group.mStats.mTotalNumReallocs, group.mStats.mTotalNumFrees, group.mName.c_str()).c_str());
                }
            }
            Print("");
        }

        // log individual leaks
        Print("--[ Memory Allocations ]----------------------------------------------------------------------------------");
        uint32 allocNumber = 0;
        for (auto& item : mAllocs)
        {
            const char*         data        = static_cast<char*>(item.first);
            const Allocation&   allocation  = item.second;

            char buffer[64];
            memset(buffer, 0, 64);

            const size_t numBytes = (allocation.mNumBytes >= 64) ? 63 : allocation.mNumBytes;
            for (uint32 i = 0; i < numBytes; ++i)
            {
                char c = data[i];
                if (c < 32 || c >= 126)
                {
                    c = '.';
                }
                buffer[i] = c;
            }

            auto catItem = mCategories.find(allocation.mCategoryID);
            MCORE_ASSERT(catItem != mCategories.end());
            Print(FormatStdString("#%-4d - %6d bytes (cat=%4d) - [%-66s] --> %s", allocNumber, allocation.mNumBytes, allocation.mCategoryID, buffer, catItem->second.mName.c_str()).c_str());

            allocNumber++;
        }
    }
}   // namespace MCore

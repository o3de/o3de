/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "MultiThreadManager.h"

// include some stl headers
// we do not use the MCore equivalent templates as they perform allocations themselves
#include <map>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>


namespace MCore
{
    /**
     * The memory tracker, used to track memory usage.
     * This class can be used to get detailed information about memory usage on a per category or global basis.
     * It can also be used to track memory leaks.
     * Internally it does not use any of MCore's functionality so that it does not polute the memory usage statistics.
     * The tracker is multithread safe, so you can call it in multiple threads.
     * Also the RegisterAlloc, RegisterRealloc and RegisterFree functions do not actually perform the allocations or frees, but are purely there for statistical tracking.
     * You can use the LogLeaks method at application shutdown to log memory leaks. To log current memory statistics you can call LogStatistics at any moment.
     */
    class MCORE_API MemoryTracker
    {
    public:
        /**
         * Information about a single allocations.
         */
        struct MCORE_API Allocation
        {
            void*   m_memAddress;            /**< The memory address of the allocation. */
            size_t  m_numBytes;              /**< The number of bytes allocated at this address. */
            uint32  m_categoryId;            /**< The memory category of this allocation. */
        };

        /**
         * The global statistics.
         */
        struct MCORE_API GlobalStats
        {
            size_t  m_currentNumBytes;       /**< Current number of bytes allocated. */
            uint32  m_currentNumAllocs;      /**< The current number of allocations. */
            uint32  m_totalNumAllocs;        /**< Total number of allocations ever made. */
            uint32  m_totalNumReallocs;      /**< Total number of reallocations ever made. */
            uint32  m_totalNumFrees;         /**< Total number of frees ever made. */

            GlobalStats();
        };

        /**
         * The per category statistics.
         */
        struct MCORE_API CategoryStats
        {
            size_t  m_currentNumBytes;       /**< Current number of bytes allocated. */
            uint32  m_currentNumAllocs;      /**< Current number of allocations active. */
            uint32  m_totalNumAllocs;        /**< Total number of allocations ever made in this category. */
            uint32  m_totalNumReallocs;      /**< Total number of reallocations ever made in this category. */
            uint32  m_totalNumFrees;         /**< Total number of frees ever made in this category. */
            std::string m_name;              /**< The name of the category, can be empty if not registered with RegisterCategory. */

            CategoryStats();
        };

        /**
         * The memory statistics for a given group.
         */
        struct MCORE_API GroupStats
        {
            size_t  m_currentNumBytes;
            uint32  m_currentNumAllocs;
            uint32  m_totalNumAllocs;        /**< Total number of allocations ever made in this category. */
            uint32  m_totalNumReallocs;      /**< Total number of reallocations ever made in this category. */
            uint32  m_totalNumFrees;         /**< Total number of frees ever made in this category. */

            GroupStats();
        };

        /**
         * A group, which is a collection of memory categories.
         */
        struct MCORE_API Group
        {
            std::set<uint32>    m_categories;    /**< The ID values of the categories that are part of this group. */
            std::string         m_name;          /**< The name of the category. */
            GroupStats          m_stats;         /**< The statistics. */
        };

        //--------------------------------

        /**
         * The constructor.
         */
        MemoryTracker();

        /**
         * The destructor.
         */
        ~MemoryTracker();

        /**
         * Register a given memory allocation.
         * This does not perform the actual allocation but just updates the internal statistics inside the memory tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param memAddress The memory address of the memory that just has been allocated and you want to register inside the tracker. This could be the address returned by a malloc for example.
         * @param numBytes The number of bytes that has been allocated.
         * @param categoryID The category ID of the memory.
         */
        void RegisterAlloc(void* memAddress, size_t numBytes, uint32 categoryID);

        /**
         * Register a reallocation of a given piece of memory.
         * This does not actually perform the realloc of the memory, but just updates the statistics inside the memory tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param oldAddress The original address of the memory that is being reallocated. When this is nullptr it will internally call RegisterAlloc instead using the newAddress as memory address.
         * @param newAddress The new address of the memory block after it has been reallocated.
         * @param numBytes The new size, in number of bytes, of the memory block that has been reallocated.
         * @param categoryID The category ID of the memory.
         */
        void RegisterRealloc(void* oldAddress, void* newAddress, size_t numBytes, uint32 categoryID);

        /**
         * Register a free of a given memory address.
         * This does not perform the release of the memory, but purely updates the statistics internally inside the tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param memAddress The memory address of the memory that is being freed.
         */
        void RegisterFree(void* memAddress);

        /**
         * Register a memory category (optional).
         * This method can be used to provide a user string based name to a given category ID.
         * You can do this at any time. If the category does not exist, it will be automatically created.
         * If it already exists because allocations have been done inside this category then it will simply update the existing name.
         * This is thread-safe and internally locks and unlocks.
         * @param categoryID The memory category identifier.
         * @param name The name you would like this category to have.
         */
        void RegisterCategory(uint32 categoryID, const char* name);

        /**
         * Reset the tracker, clearing any information about its allocations and memory categories.
         * This clears all categories as well, so it will be like no categories have been registered or internally created.
         */
        void Clear();

        /**
         * Get the global memory usage statistics.
         * This gives information like the current memory usage in total.
         * If you would like more detailed information about what categories use what amount of memory, use the GetCategoryStats method.
         * @result The global memory statistics.
         */
        const GlobalStats& GetGlobalStats() const;

        /**
         * Get statistics about a given memory category.
         * This is thread-safe and internally locks and unlocks.
         * @param categoryID The memory category ID.
         * @param outStats The statistics for this given memory category.
         * @result Returns true when the category statistics have been found and output, or false when no such category currently exists. In the case it returns false the outStats parameter remains untouched.
         */
        bool GetCategoryStatistics(uint32 categoryID, CategoryStats* outStats);

        /**
         * Register a group by providing a group ID and a list of categories that the group should track.
         * If you make multiple calls to RegisterGroup, it will merge the categories if the same group ID is provided.
         * So you can add new categories by caling this method as well.
         * If the group already exists, the name will be overwritten with the one specified.
         * This is thread-safe and internally locks and unlocks.
         * @param groupID The ID of the group you wish to register or update.
         * @param name The name of the group.
         * @param categories The list of categories that should be added to the group tracking.
         */
        void RegisterGroup(uint32 groupID, const char* name, const std::vector<uint32>& categories);

        /**
         * Get the statistics for a given group.
         * When the given groupID does not exist, this method will instantly return and outGroupStats will remain untouched.
         * Please keep in mind that the group statistics are not actively tracked. You need to call UpdateGroupStatistics to refresh the statistics for all groups.
         * You can call UpdateGroupStatistics once and then call GetGroupStatistics for each group.
         * @param groupID The unique ID of the group.
         * @param outGroupStats A pointer to the group statistics struct that will contain the statistics for this group.
         */
        bool GetGroupStatistics(uint32 groupID, GroupStats* outGroupStats);

        /**
         * Update the internal statistics for all groups.
         * This is thread-safe and internally locks and unlocks.
         */
        void UpdateGroupStatistics();

        /**
         * Log information about all allocations that have been done and are currently active.
         * The logging will NOT output to a log file, but print the data to the debug output. This can be stdout, using printf or on Windows using OutputDebugString to display it inside the Visual Studio output window.
         * This is thread-safe and internally locks and unlocks.
         * @param currentlyAllocatedOnly When set to true it will only log categories that currently have active allocations that haven't been freed yet.
         */
        void LogStatistics(bool currentlyAllocatedOnly = true);

        /**
         * Log all current active allocations as leaks.
         * This will log the leaking category stats as well as all individual allocations.
         * This should be called at application shutdown.
         * The logging will NOT output to a log file, but print the data to the debug output. This can be stdout, using printf or on Windows using OutputDebugString to display it inside the Visual Studio output window.
         * This is thread-safe and internally locks and unlocks.
         */
        void LogLeaks();

        /**
         * Get the current collection of allocations.
         * The allocations are stored in an unordered map with the memory address as key.
         * Keep in mind that for thread safety you should lock the memory tracker using the Lock method, while you read the returned data, after which you should call Unlock again.
         * @result The unordered map that contains each allocation.
         */
        const std::unordered_map<void*, Allocation>& GetAllocations() const;

        /**
         * Get the collection of registered groups.
         * The groups are stored in an unordered map with the group ID as key.
         * Keep in mind that for thread safety you should lock the memory tracker using the Lock method, while you read the returned data, after which you should call Unlock again.
         * @result The unordered map that contains all groups.
         */
        const std::unordered_map<uint32, Group>& GetGroups() const;

        /**
         * Get the collection of registered categories.
         * The categories are stored in an ordered map, ordered on category ID, which is also the key.
         * Keep in mind that for thread safety you should lock the memory tracker using the Lock method, while you read the returned data, after which you should call Unlock again.
         * @result The ordered map that contains all categories.
         */
        const std::map<uint32, CategoryStats>& GetCategories() const;

        /**
         * Lock the contents of this memory tracker using a multithread mutex.
         */
        void Lock();

        /**
         * Unlock the contents of this memory tracker using a multithread mutex.
         */
        void Unlock();

    private:
        std::unordered_map<void*, Allocation>   m_allocs;            /**< The unordered map of allocations, with the memory address as key. */
        std::unordered_map<uint32, Group>       m_groups;            /**< The groups, with the group ID as key.*/
        std::map<uint32, CategoryStats>         m_categories;        /**< The ordered map of categories, with the category ID as key. */
        GlobalStats                             m_globalStats;       /**< The global memory statistics. */
        mutable Mutex                           m_mutex;             /**< The multithread mutex used to lock and unlock. */

        /**
         * Register a given memory allocation.
         * This does not perform the actual allocation but just updates the internal statistics inside the memory tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param memAddress The memory address of the memory that just has been allocated and you want to register inside the tracker. This could be the address returned by a malloc for example.
         * @param numBytes The number of bytes that has been allocated.
         * @param categoryID The category ID of the memory.
         */
        void RegisterAllocNoLock(void* memAddress, size_t numBytes, uint32 categoryID);

        /**
         * Register a reallocation of a given piece of memory.
         * This does not actually perform the realloc of the memory, but just updates the statistics inside the memory tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param oldAddress The original address of the memory that is being reallocated. When this is nullptr it will internally call RegisterAlloc instead using the newAddress as memory address.
         * @param newAddress The new address of the memory block after it has been reallocated.
         * @param numBytes The new size, in number of bytes, of the memory block that has been reallocated.
         * @param categoryID The category ID of the memory.
         */
        void RegisterReallocNoLock(void* oldAddress, void* newAddress, size_t numBytes, uint32 categoryID);

        /**
         * Register a free of a given memory address.
         * This does not perform the release of the memory, but purely updates the statistics internally inside the tracker.
         * This is thread-safe and internally locks and unlocks.
         * @param memAddress The memory address of the memory that is being freed.
         */
        void RegisterFreeNoLock(void* memAddress);

        //
        void UpdateGroupStatisticsNoLock();

        bool GetCategoryStatisticsNoLock(uint32 categoryID, CategoryStats* outStats);
    };
} // namespace MCore

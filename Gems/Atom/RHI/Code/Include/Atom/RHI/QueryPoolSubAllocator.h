/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    namespace RHI
    {
        /**
        * Allocator for query resources's space.
        * The allocator tries to allocate all requested queries in a continuous space.
        * If that is not possible, it allocates in multiple smaller pieces, always trying
        * to group as many queries in a consecutive manner as possible.
        * This class is not thread safe.
        */
        class QueryPoolSubAllocator
        {
        public:
            struct Allocation
            {
                Allocation() = default;
                Allocation(uint32_t offset, uint32_t count)
                    : m_offset(offset)
                    , m_count(count)
                {}

                uint32_t m_offset = 0;
                uint32_t m_count = 0;
            };

            QueryPoolSubAllocator() = default;
            ~QueryPoolSubAllocator() = default;

            /**
            * Initialize the sub allocator.
            */
            void Init(uint32_t maxQueryCount);
            /**
            * Allocate space for a group of Query objects. It will try to allocate in a consecutive manner
            * if enough space is available. If not, it will allocate in multiple groups.
            * @param count Number of queries to allocate.
            * @return A list of allocations.
            */
            AZStd::vector<Allocation> Allocate(uint32_t count);
            /**
            * Free a specific space of an allocation.
            * @param offset The position that is being deallocated.
            */
            void DeAllocate(uint32_t offset);
        
        private:
            struct SortByOffset
            {
                bool operator ()(const Allocation& lhs, const Allocation& rhs) const
                {
                    return lhs.m_offset < rhs.m_offset;
                }
            };

            struct SortBySize
            {
                bool operator ()(const Allocation& lhs, const Allocation& rhs) const
                {
                    return lhs.m_count < rhs.m_count;
                }
            };

            void AddFreeSpace(const Allocation& allocation);
            void SortFreeSpaces();

            /// List of free spaces.
            AZStd::vector<Allocation> m_freeAllocations;
            /// List of allocations.
            AZStd::set<Allocation, SortByOffset> m_allocations;
            /// Total free space.
            uint32_t m_totalFreeSpace = 0;
       };
    }
}
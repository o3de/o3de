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
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "Base.h"
#include <AzCore/std/containers/vector.h>

namespace AzRHI
{
    template<typename T, size_t TableSize = 4u << 10>
    class PartitionTable
    {
    private:
        enum
        {
            TableShift = CompileTimeIntegerLog2<TableSize>::result,
            TableMask = TableSize - 1,
            TableCountMax = 0x2ff,
        };

        AZStd::allocator m_allocator;
        AZStd::vector<T*> m_storage;
        AZStd::vector<AZ::u32> m_table;
        AZStd::vector<AZ::u32> m_tableRemap;

        AZ::u32 m_size;
        AZ::u32 m_capacity;

        PartitionTable<T>& operator= (const PartitionTable<T>& rhs);
        PartitionTable(const PartitionTable<T>& rhs);

    public:
        PartitionTable<T>& operator= (PartitionTable<T>&& rhs)
        {
            m_storage = std::move(rhs.m_storage);
            m_table = std::move(rhs.m_table);
            m_tableRemap = std::move(rhs.m_tableRemap);
            m_size = std::move(rhs.m_size);
            m_capacity = std::move(rhs.m_capacity);
            rhs.m_capacity = 0;
            rhs.m_size = 0;
            return *this;
        }

        PartitionTable(PartitionTable<T>&& rhs)
            : m_storage(std::move(rhs.m_storage))
            , m_table(std::move(rhs.m_table))
            , m_tableRemap(std::move(rhs.m_tableRemap))
            , m_size(std::move(rhs.m_size))
            , m_capacity(std::move(rhs.m_capacity))
        {
            rhs.m_size = 0;
            rhs.m_capacity = 0;
        }

        PartitionTable()
            : m_storage()
            , m_table()
            , m_tableRemap()
            , m_size()
            , m_capacity()
        {
            m_storage.resize(TableCountMax);
        }

        ~PartitionTable()
        {
            Clear();
        }

        void Clear()
        {
            for (AZ::u32 rosterIndex = 0; rosterIndex < m_size; ++rosterIndex)
            {
                AZ::u32 key = m_table[rosterIndex];
                size_t table = key >> TableShift;
                size_t index = key & TableMask;
                (&(m_storage[table][index]))->~T();
            }

            for (T* partition : m_storage)
            {
                m_allocator.deallocate(partition, TableSize * sizeof(T), 16);
            }

            m_storage.clear();
            m_table.clear();
            m_tableRemap.clear();
            m_size = 0;
            m_capacity = 0;
        }

        inline AZ::u32 Capacity() const
        {
            return m_capacity;    
        }

        inline AZ::u32 Count() const
        {
            return m_size;
        }

        inline T& operator[] (size_t key)
        {
            size_t table = key >> TableShift;
            size_t index = key & TableMask;
            return m_storage[table][index];
        }

        inline const T& operator[] (size_t key) const
        {
            size_t table = key >> TableShift;
            size_t index = key & TableMask;
            return m_storage[table][index];
        }

        AZ::u32 Allocate()
        {
            if (m_size + 1 >= m_capacity)
            {
                size_t oldCapacity = m_capacity;
                m_capacity += TableSize;

                // Create a new table of TableSize elements
                AZ_Assert((m_capacity >> TableShift) <= TableCountMax, "exceeded capacity, increase TableCountMax");

                m_storage[oldCapacity >> TableShift] = (T*)m_allocator.allocate(TableSize * sizeof(T), 16);
                m_table.resize(m_capacity, AZ::u32());

                // Append new indices to the free list
                std::iota(m_table.begin() + oldCapacity, m_table.end(), oldCapacity);
                m_tableRemap.resize(m_capacity, AZ::u32());
            }

            size_t rosterIndex = m_size++;
            AZ::u32 key = m_table[rosterIndex];
            m_tableRemap[key] = rosterIndex;

            new (&(this->operator[](key))) T(key);
            return key;
        }

        inline void Free(AZ::u32 key)
        {
            AZRHI_ASSERT(m_size && key < m_tableRemap.size());

            AZ::u32 rosterIndex = m_tableRemap[key];
            (&(this->operator[](key)))->~T();

            // Defragment the backing tables
            std::swap(m_table[rosterIndex], m_table[--m_size]);
            std::swap(m_tableRemap[key], m_tableRemap[m_table[rosterIndex]]);
        }
    };
}

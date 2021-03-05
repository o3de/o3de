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
namespace AzRHI
{
    //////////////////////////////////////////////////////////////////////////
    // Partition based allocator for constant buffers of roughly the same size
    struct PartitionAllocator
    {
        D3DBuffer* m_buffer;
        void* m_base_ptr;
        uint32 m_page_size;
        uint32 m_bucket_size;
        uint32 m_partition;
        uint32 m_capacity;

        std::vector<uint32> m_table;
        std::vector<uint32> m_remap;

        PartitionAllocator(
            D3DBuffer* buffer, void* base_ptr, size_t page_size, size_t bucket_size)
            : m_buffer(buffer)
            , m_base_ptr(base_ptr)
            , m_page_size((uint32)page_size)
            , m_bucket_size((uint32)bucket_size)
            , m_partition(0)
            , m_capacity(page_size / bucket_size)
            , m_table()
            , m_remap()
        {
            m_table.resize(page_size / bucket_size);
            m_remap.resize(page_size / bucket_size);
            std::iota(m_table.begin(), m_table.end(), 0);
        }
        ~PartitionAllocator()
        {
            DEVBUFFERMAN_VERIFY(m_partition == 0);
            UnsetStreamSources(m_buffer);
            SAFE_RELEASE(m_buffer);
        }

        D3DBuffer* buffer() const { return m_buffer; }
        void* base_ptr() const { return m_base_ptr; }
        bool empty() const { return m_partition == 0; }

        uint32 allocate()
        {
            size_t key = ~0u;
            IF (m_partition + 1 >= m_capacity, 0)
            {
                return ~0u;
            }
            uint32 storage_index = m_table[key = m_partition++];
            m_remap[storage_index] = key;
            return storage_index;
        }

        void deallocate(size_t key)
        {
            DEVBUFFERMAN_ASSERT(m_partition && key < m_remap.size());
            uint32 roster_index = m_remap[key];
            std::swap(m_table[roster_index], m_table[--m_partition]);
            std::swap(m_remap[key], m_remap[m_table[roster_index]]);
        }
    };
}

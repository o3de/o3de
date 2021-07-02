/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_PARALLEL_CONTAINERS_CONCURRENT_VECTOR_H
#define AZSTD_PARALLEL_CONTAINERS_CONCURRENT_VECTOR_H 1

#include <AzCore/std/allocator.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZStd
{
    /**
     * This container is lock free, provides indexed access and is dynamically resizable.
     * It will not free up memory unless clear() is explicitly called.
     *
     * WARNING 1: concurrent_vector IS NOT THREADSAFE!
     * The size of the vector is changed atomically, and memory allocations are atomic, but
     * elements are not pushed onto the list atomically when the size changes. If you just
     * want a list that dynamically grows, doesn't free up and reallocate existing pointers,
     * and can be read from in a thread-safe way, then use this.
     * Otherwise, use something else (like an AZStd::list with a mutex).
     *
     * WARNING 2: constructors and destructors on elements created by this will
     * not be called. Best to use with structs and simple data types (ints, floats, etc)
     */
    template<typename T, typename Allocator = AZStd::allocator,
        unsigned int INITIAL_CAPACITY_LOG = 5, unsigned int MAX_CAPACITY_LOG = 32>
    class concurrent_vector
    {
    public:
        concurrent_vector()
        {
            m_size.store(0, memory_order_release);
            for (unsigned int i = 0; i < MAX_NUM_CHUNKS; ++i)
            {
                m_chunks[i].store(NULL, memory_order_release);
            }
        }

        ~concurrent_vector()
        {
            for (unsigned int i = 0; i < MAX_NUM_CHUNKS; ++i)
            {
                T* chunk = m_chunks[i].load(memory_order_acquire);
                if (chunk)
                {
                    unsigned int chunkSize = GetChunkSize(i);
                    m_alloc.deallocate(chunk, chunkSize * sizeof(T), alignment_of<T>::value);
                }
            }
        }

        const T& operator[](unsigned int i) const
        {
            return (*const_cast<concurrent_vector<T, Allocator, INITIAL_CAPACITY_LOG, MAX_CAPACITY_LOG>*>(this))[i];
        }

        T& operator[](unsigned int i)
        {
            AZ_Assert(i < m_size, ("Index out of range"));
            unsigned int chunkIndex = GetChunkForIndex(i);
            unsigned int chunkSize = GetChunkSize(chunkIndex);
            T* chunk = GetChunk(chunkIndex);
            return chunk[i & (chunkSize - 1)];
        }

        ///Returns index of element that was added
        unsigned int push_back(const T& v)
        {
            unsigned int index = m_size.fetch_add(1, memory_order_acq_rel);
            (*this)[index] = v;
            return index;
        }

        bool empty() const        { return (m_size.load(memory_order_acquire) == 0); }

        unsigned int size() const { return m_size.load(memory_order_acquire); }

        void clear()
        {
            m_size.store(0, memory_order_release);
        }

        void resize(unsigned int newSize)
        {
            m_size.store(newSize, memory_order_release);
        }

    private:
        unsigned int GetChunkForIndex(unsigned int index) const
        {
            //find highest set bit in the bucket index
            unsigned int highestBitSet = INITIAL_CAPACITY_LOG - 1;
            unsigned int v = index >> INITIAL_CAPACITY_LOG;
            while (v)
            {
                ++highestBitSet;
                v >>= 1;
            }

            unsigned int chunkIndex = highestBitSet - (INITIAL_CAPACITY_LOG - 1);
            AZ_Assert(chunkIndex < MAX_NUM_CHUNKS, ("Ran out of chunks"));
            return chunkIndex;
        }

        unsigned int GetChunkSize(unsigned int chunkIndex) const
        {
            //each chunk is double the size of the previous one
            //first chunk is an exception, it is twice as big because it contains storage for the missing small chunks too
            if (chunkIndex == 0)
            {
                return 1 << INITIAL_CAPACITY_LOG;
            }
            else
            {
                return 1 << (chunkIndex + INITIAL_CAPACITY_LOG - 1);
            }
        }

        T* GetChunk(unsigned int chunkIndex)
        {
            T* chunk = m_chunks[chunkIndex].load(memory_order_acquire);
            if (!chunk)
            {
                return AllocateChunk(chunkIndex);
            }
            return chunk;
        }

        T* AllocateChunk(unsigned int chunkIndex)
        {
            //allocate a new chunk, and attempt to assign it atomically with a compareAndSwap
            unsigned int chunkSize = GetChunkSize(chunkIndex);
            T* newChunk = static_cast<T*>(m_alloc.allocate(chunkSize * sizeof(T), alignment_of<T>::value));
            T* oldChunk = NULL;
            if (!m_chunks[chunkIndex].compare_exchange_strong(oldChunk, newChunk, memory_order_acq_rel, memory_order_acquire))
            {
                //somebody beat us to it, that's ok, use their chunk and free our attempted allocation
                m_alloc.deallocate(newChunk, chunkSize * sizeof(T), alignment_of<T>::value);
                return oldChunk;
            }
            return newChunk;
        }

        static const unsigned int MAX_NUM_CHUNKS = MAX_CAPACITY_LOG - (INITIAL_CAPACITY_LOG - 1);
        atomic<T*> m_chunks[MAX_NUM_CHUNKS];

        atomic<unsigned int> m_size;
        Allocator m_alloc;
    };
}

#endif
#pragma once

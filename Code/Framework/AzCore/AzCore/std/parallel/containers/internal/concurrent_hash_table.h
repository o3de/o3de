/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/hash.h> //for primes list
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/allocator_ref.h>
#include <AzCore/std/parallel/allocator_concurrent_static.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZStd
{
    namespace Internal
    {
        /**
         * Storage helper class for concurrent_hash_table, this is the dynamic storage specialization
         */
        template<class Traits, bool IsDynamic = Traits::is_dynamic>
        class concurrent_hash_table_storage
        {
            typedef concurrent_hash_table_storage<Traits, IsDynamic> this_type;

        public:
            typedef typename Traits::allocator_type allocator_type;
            typedef AZStd::list<typename Traits::value_type, allocator_type> list_type;
            typedef typename list_type::size_type size_type;

            typedef list_type vector_value_type;
            typedef AZStd::vector<vector_value_type, allocator_type> vector_type;

            concurrent_hash_table_storage(const allocator_type& alloc)
                : m_vector(alloc)
            {
                init_buckets();
                max_load_factor(Traits::max_load_factor);
            }

            concurrent_hash_table_storage(const this_type& rhs)
                : m_vector(rhs.m_vector.get_allocator())
            {
                init_buckets();
                max_load_factor(rhs.max_load_factor());
            }

            void init_buckets()
            {
                m_buckets = &m_startBucket;
                m_numBuckets.store(1, memory_order_release);
            }

            inline void pre_copy(const this_type& rhs)
            {
                m_vector.assign(rhs.get_num_buckets(), vector_value_type(0));
                m_numBuckets.store(rhs.get_num_buckets(), memory_order_release);
                m_maxLoadFactor.store(rhs.max_load_factor(), memory_order_release);
                m_buckets = &m_vector[0];
                m_startBucket.clear();
            }

            AZ_FORCE_INLINE vector_value_type* buckets()             { return m_buckets; }
            AZ_FORCE_INLINE const vector_value_type* buckets() const { return m_buckets; }

            AZ_FORCE_INLINE size_type get_num_buckets()       { return m_numBuckets.load(memory_order_acquire); }
            AZ_FORCE_INLINE size_type get_num_buckets() const { return m_numBuckets.load(memory_order_acquire); }

            float max_load_factor() const { return m_maxLoadFactor.load(memory_order_acquire); }
            void max_load_factor(float newMaxLoadFactor)
            {
                AZSTD_CONTAINER_ASSERT(newMaxLoadFactor > 0.0f, "AZStd::hash_table_storage::max_load_factor - invalid hash load factor");
                m_maxLoadFactor.store(newMaxLoadFactor, memory_order_release);
            }

            template<class HashTable>
            void rehash(HashTable* table, size_type minNumBuckets)
            {
                size_type numBuckets = hash_next_bucket_size(minNumBuckets);

                vector_type newBuckets(numBuckets, vector_value_type(), m_vector.get_allocator());

                size_type oldNumBuckets = get_num_buckets();
                for (size_type bucketIndex = 0; bucketIndex < oldNumBuckets; ++bucketIndex)
                {
                    list_type& bucket = m_buckets[bucketIndex];

                    typename list_type::iterator iter = bucket.begin();
                    while (iter != bucket.end())
                    {
                        typename list_type::iterator iterNext = iter;
                        ++iterNext;

                        typename HashTable::key_type valueKey = Traits::key_from_value(bucket.front());
                        size_type newBucketIndex = table->m_hasher(valueKey) % numBuckets;
                        vector_value_type& newBucket = newBuckets[newBucketIndex];
                        newBucket.splice(newBucket.begin(), bucket, iter, iterNext);
                        iter = iterNext;
                    }
                }

                m_vector.swap(newBuckets);
                m_numBuckets.store(numBuckets, memory_order_release);
                m_buckets = &m_vector[0];
                m_startBucket.clear();
            }

            void clear()
            {
                m_vector.assign(Traits::min_buckets, vector_value_type());
                m_numBuckets.store(Traits::min_buckets, memory_order_release);
                m_buckets = &m_vector[0];
                m_startBucket.clear();
            }

            void swap(this_type& rhs)
            {
                m_vector.swap(rhs.m_vector);

                AZStd::swap(m_startBucket, rhs.m_startBucket);
                AZStd::swap(m_buckets, rhs.m_buckets);

                size_type tempNumBuckets = m_numBuckets.load(memory_order_acquire);
                m_numBuckets.store(rhs.m_numBuckets.load(memory_order_acquire), memory_order_release);
                rhs.m_numBuckets.store(tempNumBuckets, memory_order_release);

                float tempLoadFactor = m_maxLoadFactor.load(memory_order_acquire);
                m_maxLoadFactor.store(rhs.m_maxLoadFactor.load(memory_order_acquire), memory_order_release);
                rhs.m_maxLoadFactor.store(tempLoadFactor, memory_order_release);
            }

        private:
            atomic<size_type> m_numBuckets; //we can read this without holding a lock, so can't use m_vector.size()
            atomic<float> m_maxLoadFactor;
            vector_value_type m_startBucket;
            vector_value_type* m_buckets;
            vector_type m_vector;
        };

        /**
         * Storage helper class for concurrent_hash_table, this is the static storage specialization
         */
        template<class Traits>
        class concurrent_hash_table_storage<Traits, false>
        {
            typedef concurrent_hash_table_storage<Traits, false> this_type;

        public:
            typedef static_pool_concurrent_allocator<typename Internal::list_node<typename Traits::value_type>,Traits::fixed_num_elements> element_allocator_type;

            typedef typename Traits::allocator_type allocator_type;
            typedef AZStd::list<typename Traits::value_type, allocator_ref<element_allocator_type> > list_type;
            typedef typename list_type::size_type size_type;

            typedef list_type vector_value_type;
            typedef AZStd::fixed_vector<vector_value_type, Traits::fixed_num_buckets> vector_type;

            concurrent_hash_table_storage(const allocator_type&)
            {
                init_buckets();
            }

            concurrent_hash_table_storage(const this_type&)
            {
                init_buckets();
            }

            void init_buckets()
            {
                m_vector.assign(Traits::fixed_num_buckets, list_type(allocator_ref<element_allocator_type>(m_elementAllocator)));
            }

            AZ_FORCE_INLINE void pre_copy(const this_type&)
            {
                init_buckets();
            }

            AZ_FORCE_INLINE vector_value_type* buckets()             { return &m_vector[0]; }
            AZ_FORCE_INLINE const vector_value_type* buckets() const { return &m_vector[0]; }

            AZ_FORCE_INLINE size_type get_num_buckets()       { return Traits::fixed_num_buckets; }
            AZ_FORCE_INLINE size_type get_num_buckets() const { return Traits::fixed_num_buckets; }

            float max_load_factor() const { return 0.0f; }
            void max_load_factor(float)   { }

            template<class HashTable>
            void rehash(HashTable*, size_type)
            {
            }

            void clear()
            {
                init_buckets();
            }

            void swap(this_type& rhs)
            {
                m_vector.swap(rhs.m_vector);
            }

        private:
            element_allocator_type m_elementAllocator;
            vector_type m_vector;
        };

        /**
         * Base implementation class for all concurrent_unordered_* containers.
         */
        template<typename Traits>
        class concurrent_hash_table
        {
            typedef concurrent_hash_table<Traits> this_type;
            typedef concurrent_hash_table_storage<Traits> storage_type;
            friend class Internal::concurrent_hash_table_storage<Traits>;
        public:
            typedef typename Traits::key_type       key_type;
            typedef typename Traits::key_equal      key_equal;
            typedef typename Traits::hasher         hasher;

            typedef typename Traits::allocator_type                 allocator_type;
            typedef typename storage_type::list_type                list_type;

            typedef typename list_type::size_type                   size_type;
            typedef typename list_type::difference_type             difference_type;
            typedef typename list_type::pointer                     pointer;
            typedef typename list_type::const_pointer               const_pointer;
            typedef typename list_type::reference                   reference;
            typedef typename list_type::const_reference             const_reference;

            typedef typename list_type::value_type                  value_type;

            typedef typename storage_type::vector_value_type        vector_value_type;
            typedef typename storage_type::vector_type              vector_type;

            AZ_FORCE_INLINE explicit concurrent_hash_table(const hasher& hash, const key_equal& keyEqual, const allocator_type& alloc = allocator_type())
                : m_storage(alloc)
                , m_keyEqual(keyEqual)
                , m_hasher(hash)
            {
                m_numElements.store(0, memory_order_release);
            }

            AZ_FORCE_INLINE concurrent_hash_table(const this_type& rhs)
                : m_storage(rhs.m_storage)
                , m_keyEqual(rhs.m_keyEqual)
                , m_hasher(rhs.m_hasher)
            {
                copy(rhs);
            }

            AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
            {
                if (this != &rhs)
                {
                    copy(rhs);
                }
                return *this;
            }

            size_type size() const { return m_numElements.load(memory_order_acquire); }
            bool empty() const     { return (size() == 0); }

            key_equal key_eq() const  { return m_keyEqual; }
            hasher get_hasher() const { return m_hasher; }

            float max_load_factor() const                { return m_storage.max_load_factor(); }
            void max_load_factor(float newMaxLoadFactor) { m_storage.max_load_factor(newMaxLoadFactor); }

            bool insert(const value_type& value)
            {
                // try to insert (possibly existing) node with value value
                key_type valueKey = Traits::key_from_value(value);
                size_type bucketIndex = acquire_bucket(m_hasher(valueKey));
                vector_value_type& bucket = m_storage.buckets()[bucketIndex];

                if constexpr (!Traits::has_multi_elements)
                {
                    for (typename list_type::const_iterator iter = bucket.begin(); iter != bucket.end(); ++iter)
                    {
                        if (m_keyEqual(valueKey, Traits::key_from_value(*iter)))
                        {
                            //TODO rbbaklov or zolniery look into why this worked previously
                            release_bucket(bucketIndex);
                            return false;
                        }
                    }
                }

                bucket.push_back(value);

                m_numElements.fetch_add(1, memory_order_acq_rel);

                release_bucket(bucketIndex);

                rehash_if_needed();

                return true;
            }

            size_type erase(const key_type& keyValue)
            {
                size_type bucketIndex = acquire_bucket(m_hasher(keyValue));
                vector_value_type& bucket = m_storage.buckets()[bucketIndex];

                size_type numFound = 0;
                typename vector_value_type::iterator iter = bucket.begin();
                while (iter != bucket.end())
                {
                    if (m_keyEqual(keyValue, Traits::key_from_value(*iter)))
                    {
                        iter = bucket.erase(iter);
                        ++numFound;
                    }
                    else
                    {
                        ++iter;
                    }
                }

                m_numElements.fetch_sub(numFound, memory_order_acq_rel);

                release_bucket(bucketIndex);

                return numFound;
            }

            bool erase_one(const key_type& keyValue)
            {
                size_type bucketIndex = acquire_bucket(m_hasher(keyValue));
                vector_value_type& bucket = m_storage.buckets()[bucketIndex];

                bool isFound = false;
                typename vector_value_type::iterator iter = bucket.begin();
                while (iter != bucket.end())
                {
                    if (m_keyEqual(keyValue, Traits::key_from_value(*iter)))
                    {
                        iter = bucket.erase(iter);
                        m_numElements.fetch_sub(1, memory_order_acq_rel);
                        isFound = true;
                        break;
                    }
                    ++iter;
                }

                release_bucket(bucketIndex);

                return isFound;
            }

            bool find(const key_type& keyValue) const
            {
                value_type dummy;
                return find(keyValue, &dummy);
            }

            void rehash(size_type numBucketsMin)
            {
                if (!Traits::is_dynamic)
                {
                    return;
                }

                acquire_all();
                size_type numElements = m_numElements.load(memory_order_acquire);
                float maxLoadFactor = max_load_factor();
                size_type loadMinNumBuckets = (size_type)((float)numElements / maxLoadFactor);
                m_storage.rehash(this, AZStd::GetMax(loadMinNumBuckets, numBucketsMin));
                release_all();
            }

            void clear()
            {
                acquire_all();
                m_storage.clear();
                m_numElements.store(0, memory_order_release);
                release_all();
            }

            void swap(this_type& rhs)
            {
                if (this == &rhs)
                {
                    return;
                }

                acquire_all();
                rhs.acquire_all();

                m_storage.swap(rhs.m_storage);

                size_type tempNumElements = m_numElements.load(memory_order_acquire);
                m_numElements.store(rhs.m_numElements.load(memory_order_acquire), memory_order_release);
                rhs.m_numElements.store(tempNumElements, memory_order_release);

                release_all();
                rhs.release_all();
            }

        protected:
            void acquire_all() const
            {
                for (unsigned int i = 0; i < num_locks; ++i)
                {
                    const_cast<mutex&>(m_locks[i]).lock();
                }
            }

            void release_all() const
            {
                for (unsigned int i = 0; i < num_locks; ++i)
                {
                    const_cast<mutex&>(m_locks[i]).unlock();
                }
            }

            size_t acquire_bucket(size_t key) const
            {
                //Tricky stuff here. This would be simplified if numBuckets was always a power of 2, then we could
                //just compute lockIndex=key&mask and be done, as growing numBuckets during the lockIndex calculation
                //would not affect the result. But restricting numBuckets to powers of 2 instead of primes puts
                //additional requirements on the hasher, so instead we will use the following method which detects
                //a resize and then releases and re-acquires the correct lock.

                size_t numBuckets = m_storage.get_num_buckets();
                size_t bucketIndex = key % numBuckets;
                size_t lockIndex = bucketIndex & lock_mask;
                const_cast<mutex&>(m_locks[lockIndex]).lock();

                //great, we now have a lock, further resizes are blocked, but is it the right one? Check if there was
                //a resize while we were calculating the lockIndex
                size_t newNumBuckets = m_storage.get_num_buckets();
                while (newNumBuckets != numBuckets)
                {
                    //release the old lock. Don't be tempted to do this later, after acquiring the correct lock,
                    //deadlocks will result if locks are not acquired in a consistent order!
                    const_cast<mutex&>(m_locks[lockIndex]).unlock();

                    //recalculate the lockIndex
                    numBuckets = newNumBuckets;
                    bucketIndex = key % numBuckets;
                    lockIndex = bucketIndex & lock_mask;

                    //and acquire what we hope is the correct lock
                    const_cast<mutex&>(m_locks[lockIndex]).lock();

                    //check for resize again and repeat
                    newNumBuckets = m_storage.get_num_buckets();
                }

                return bucketIndex;
            }

            void release_bucket(size_t bucketIndex) const
            {
                size_t lockIndex = bucketIndex & lock_mask;
                const_cast<mutex&>(m_locks[lockIndex]).unlock();
            }

            void copy(const this_type& rhs)
            {
                acquire_all();
                rhs.acquire_all();

                m_storage.pre_copy(rhs.m_storage);

                for (size_type iBucket = 0; iBucket < rhs.m_storage.get_num_buckets(); ++iBucket)
                {
                    const vector_value_type& srcBucket = rhs.m_storage.buckets()[iBucket];
                    vector_value_type& destBucket = m_storage.buckets()[iBucket];
                    destBucket = srcBucket;
                }

                m_numElements.store(rhs.size(), memory_order_release);

                release_all();
                rhs.release_all();
            }

            void rehash_if_needed()
            {
                if constexpr (!Traits::is_dynamic)
                {
                    return;
                }
                else
                {
                    float loadFactor = (float)m_numElements.load(memory_order_acquire) / (float)m_storage.get_num_buckets();
                    if (loadFactor > max_load_factor())
                    {
                        acquire_all();

                        // check the load factor again, as another thread may have beaten us to the rehash
                        size_type numElements = m_numElements.load(memory_order_acquire);
                        float maxLoadFactor = max_load_factor();
                        size_type numBuckets = m_storage.get_num_buckets();
                        loadFactor = (float)numElements / (float)numBuckets;
                        if (loadFactor > maxLoadFactor)
                        {
                            size_type minNumBuckets = (size_type)((float)numElements / maxLoadFactor);
                            m_storage.rehash(this, minNumBuckets);
                        }

                        release_all();
                    }
                }
            }

            bool find(const key_type& keyValue, value_type* valueOut) const
            {
                size_type bucketIndex = acquire_bucket(m_hasher(keyValue));
                const vector_value_type& bucket = m_storage.buckets()[bucketIndex];

                bool isFound = false;
                for (typename list_type::const_iterator iter = bucket.begin(); iter != bucket.end(); ++iter)
                {
                    if (m_keyEqual(keyValue, Traits::key_from_value(*iter)))
                    {
                        *valueOut = *iter;
                        isFound = true;
                        break;
                    }
                }

                release_bucket(bucketIndex);

                return isFound;
            }

            enum
            {
                num_locks = Traits::num_locks,
                lock_mask = num_locks - 1
            };
            static_assert((num_locks & (num_locks - 1)) == 0, "must be power of 2");

            storage_type m_storage;
            key_equal m_keyEqual;
            hasher m_hasher;
            atomic<size_type> m_numElements;
            mutex m_locks[num_locks];
        };
    }
}


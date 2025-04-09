/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/containers_concepts.h>
#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/fixed_list.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/allocator_ref.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/typetraits/is_convertible.h>

namespace AZStd
{
    template<class Traits>
    class hash_table;

    namespace Internal
    {
        /**
         * Hash table data storage class. It's different for dynamic and fixed hash tables.
         * it's considered part of the hash table. Encapsulation is avoided to avoid too many function calls.
         */
        template<class Traits, bool IsDynamic = Traits::is_dynamic>
        class hash_table_storage
        {
            typedef hash_table_storage<Traits, IsDynamic> this_type;

        public:
            typedef typename Traits::allocator_type                                                 allocator_type;
            typedef AZStd::list<typename Traits::value_type, AZStd::allocator_ref<allocator_type> >  list_type;
            typedef typename list_type::size_type                                                   size_type;

            typedef AZStd::pair<size_type, typename list_type::iterator>                             vector_value_type;
            typedef AZStd::vector<vector_value_type, AZStd::allocator_ref<allocator_type> >          vector_type;

            AZ_FORCE_INLINE hash_table_storage(const allocator_type& alloc)
                : m_allocator(alloc)
                , m_list(typename list_type::allocator_type(m_allocator))
                , m_vector(typename vector_type::allocator_type(m_allocator))
                , m_max_load_factor(Traits::max_load_factor)
            {
                init_buckets();
            }

            hash_table_storage(const this_type& rhs)
                : hash_table_storage(rhs, rhs.m_allocator)
            {
            }
            hash_table_storage(const this_type& rhs, const type_identity_t<allocator_type>& alloc)
                : m_allocator(alloc)
                , m_list(typename list_type::allocator_type(m_allocator))
                , m_vector(typename vector_type::allocator_type(m_allocator))
                , m_max_load_factor(rhs.m_max_load_factor)
            {
                init_buckets();
            }

            AZ_FORCE_INLINE float           max_load_factor() const     {   return m_max_load_factor;   }
            AZ_FORCE_INLINE void            max_load_factor(float newMaxLoadFactor)
            {
                AZSTD_CONTAINER_ASSERT(newMaxLoadFactor > 0.0f, "AZStd::hash_table_storage::max_load_factor - invalid hash load factor");
                m_max_load_factor = newMaxLoadFactor;
            }

            AZ_FORCE_INLINE vector_value_type* buckets()            { return m_buckets; }
            AZ_FORCE_INLINE vector_value_type* buckets()    const   { return m_buckets; }

            AZ_FORCE_INLINE size_type         get_num_buckets()             { return m_numBuckets; }
            AZ_FORCE_INLINE size_type         get_num_buckets() const       { return m_numBuckets; }

            void swap(this_type& rhs)
            {
                m_list.swap(rhs.m_list);
                m_vector.swap(rhs.m_vector);

                AZStd::swap(m_startBucket.first, rhs.m_startBucket.first);
                m_startBucket.second = m_list.begin();
                rhs.m_startBucket.second = rhs.m_list.begin();
                AZStd::swap(m_numBuckets, rhs.m_numBuckets);
                AZStd::swap(m_max_load_factor, rhs.m_max_load_factor);

                if (m_vector.empty())
                {
                    m_buckets = &m_startBucket;
                }
                else
                {
                    m_buckets = &m_vector[0];
                }

                if (rhs.m_vector.empty())
                {
                    rhs.m_buckets = &rhs.m_startBucket;
                }
                else
                {
                    rhs.m_buckets = &rhs.m_vector[0];
                }
            }

            AZ_FORCE_INLINE void init_buckets()
            {
                m_vector.set_capacity(0); // make sure we free any bucket memory.
                m_buckets       = &m_startBucket;
                m_numBuckets    = 1;
                m_startBucket.first = 0;
                m_startBucket.second = m_list.end();
            }

            void pre_copy(const this_type& rhs)
            {
                m_vector.assign(rhs.m_vector.size(), vector_value_type(0));
                m_numBuckets = rhs.m_numBuckets;
                m_max_load_factor = rhs.m_max_load_factor;
                m_list.clear();

                if (m_vector.empty())
                {
                    init_buckets();
                }
                else
                {
                    m_buckets = &m_vector[0];
                }
            }

            template<class HashTable>
            void        rehash(HashTable* table, size_type numBucketsMin)
            {
                size_type num_buckets = 0;

                numBucketsMin = (AZStd::max)(numBucketsMin, (size_type)ceilf((float)m_list.size() / m_max_load_factor));

                if (numBucketsMin != 0)
                {
                    num_buckets = hash_next_bucket_size(numBucketsMin);
                }

                if (num_buckets == m_numBuckets)
                {
                    return; // no need yet to rehash
                }
                m_numBuckets = num_buckets;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                Debug::checked_iterator_base* debugIterList;
                {
                    // Detach all current iterators. They will be valid after.
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                    AZ_GLOBAL_SCOPED_LOCK(m_list.get_global_section());
#endif
                    debugIterList = m_list.m_iteratorList;
                    m_list.m_iteratorList = 0;
                }
#endif

                list_type newList(m_list.get_allocator());
                vector_type newBuckets(num_buckets, vector_value_type(0), m_vector.get_allocator());
                typename list_type::iterator cur, last = m_list.end();
                while (!m_list.empty())
                {
                    cur = m_list.begin();
                    typename list_type::iterator insertIter, curEnd(cur);
                    const typename HashTable::key_type& valueKey = Traits::key_from_value(*cur);

                    size_type numValues = 1;
                    // Get the number of same consecutive elements in the table with same key,
                    // this allows range insertion of elements at once
                    for (++curEnd; curEnd != last && table->m_keyEqual(valueKey, Traits::key_from_value(*curEnd)); ++curEnd, ++numValues)
                    {
                    }

                    size_type newBucketIndex = table->bucket_from_hash(table->m_hasher(valueKey));

                    // newBucket.first holds the total number of elements in the bucket
                    // newBucket.second contains the pointer to the first element in the bucket
                    vector_value_type& newBucket = newBuckets[newBucketIndex];
                    size_type numElements = newBucket.first;
                    insertIter = newBucket.second;

                    // If we don't have elements in the bucket yet, transfer the elements directly
                    if (numElements == 0)
                    {
                        newList.splice(newList.begin(), m_list, cur, curEnd);
                        newBucket.second = newList.begin();
                    }
                    else
                    {
                        // Since there are elements already in the bucket, update `insertIter` to where the elements will need to be inserted.
                        if (!table->find_insert_position(valueKey, table->m_keyEqual, insertIter, numElements, integral_constant<bool, Traits::has_multi_elements>()))
                        {
                            // An element was found but we don't allow for duplicate elements in this table.
                            // This happens when there was an insertion of two elements that are equal but have different hashes,
                            // which is undefined behavior for a hash table: ISO C++ N4713, section 23.14.15 - 5.3
                            AZ_Assert(false, "Found a duplicate element when rehashing. "
                                "Review the hashing function for this type and make sure two equal elements always have the same hash");
                        }

                        newList.splice(insertIter, m_list, cur, curEnd);
                    }

                    newBucket.first += numValues;
                }

                m_list.swap(newList);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                {
                    // All iterators will be still valid.
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
                    AZ_GLOBAL_SCOPED_LOCK(m_list.get_global_section());
#endif
                    if (debugIterList != 0)
                    {
                        // attach old iterators at the end
                        Debug::checked_iterator_base* last = m_list.m_iteratorList;
                        if (last)
                        {
                            while (last->m_nextIterator != 0)
                            {
                                last = last->m_nextIterator;
                            }
                            last->m_nextIterator = debugIterList;
                            debugIterList->m_prevIterator = last;
                        }
                        else
                        {
                            m_list.m_iteratorList = debugIterList;
                        }
                    }
                }
#endif
                m_vector.swap(newBuckets);
                if (m_vector.empty())
                {
                    init_buckets();
                }
                else
                {
                    m_buckets = &m_vector[0];
                }
            }

            template<class HashTable>
            AZ_FORCE_INLINE void         rehash_if_needed(HashTable* table)
            {
                float loadFactor = (float)m_list.size() / (float)m_numBuckets;
                if (m_max_load_factor < loadFactor)
                {
                    rehash(table, m_numBuckets + 1);   // too dense, need to grow hash table
                }
            }

            void        set_allocator(const allocator_type& alloc)
            {
                m_allocator = alloc;
                m_list.set_allocator(typename list_type::allocator_type(&m_allocator));
                m_vector.set_allocator(typename vector_type::allocator_type(&m_allocator));
            }

            allocator_type  m_allocator;    //!< The single instance of the allocator shared between list and vector containers.
            list_type       m_list;         //!< List with elements.
            vector_type     m_vector;       //!< Buckets with list iterators.

        private:
            vector_value_type* m_buckets;       //!< Current buckets array. (can point to the m_vector or m_startBucket).
            size_type           m_numBuckets;   //!< Current number of buckets.
            float               m_max_load_factor; //!< Maximum load (elements/buckets) before rehashing.
            vector_value_type   m_startBucket;  //!< Start bucket used for before we start dynamically allocate memory from m_vector.
        };

        /**
        * Hash table data storage class. It's different for dynamic and fixed hash tables.
        * it's considered part of the hash table. Encapsulation is avoided to avoid too many function calls.
        */
        template<class Traits>
        class hash_table_storage<Traits, false>
        {
            typedef hash_table_storage this_type;

        public:
            typedef typename Traits::allocator_type                                     allocator_type;
            typedef AZStd::fixed_list<typename Traits::value_type, Traits::fixed_num_elements>  list_type;
            typedef typename list_type::size_type   size_type;

            typedef AZStd::pair<size_type, typename list_type::iterator>                 vector_value_type;
            typedef AZStd::fixed_vector<vector_value_type, Traits::fixed_num_buckets>    vector_type;

            AZ_FORCE_INLINE hash_table_storage(const allocator_type&)
            {
                init_buckets();
            }

            AZ_FORCE_INLINE hash_table_storage(const this_type&)
            {
                init_buckets();
            }

            AZ_FORCE_INLINE float           max_load_factor() const                 { return (float)m_vector.size() / (float)m_vector.capacity(); }
            AZ_FORCE_INLINE void            max_load_factor(float newMaxLoadFactor) { (void)newMaxLoadFactor; }

            AZ_FORCE_INLINE vector_value_type*          buckets()           { return &m_vector[0]; }
            AZ_FORCE_INLINE const vector_value_type*    buckets()   const   { return &m_vector[0]; }

            AZ_FORCE_INLINE size_type         get_num_buckets()             { return m_vector.size(); }
            AZ_FORCE_INLINE size_type         get_num_buckets() const       { return m_vector.size(); }

            AZ_INLINE void swap(this_type& rhs)
            {
                m_list.swap(rhs.m_list);
                m_vector.swap(rhs.m_vector);
            }

            AZ_FORCE_INLINE void init_buckets()
            {
                m_vector.assign(m_vector.capacity(), vector_value_type(0));
            }

            AZ_FORCE_INLINE void pre_copy(const this_type&)
            {
                m_vector.assign(m_vector.capacity(), vector_value_type(0));
                m_list.clear();
            }

            template<class HashTable>
            AZ_FORCE_INLINE void    rehash_if_needed(HashTable*)    {}

            template<class HashTable>
            AZ_FORCE_INLINE void    rehash(HashTable*, size_type) {}

            vector_type     m_vector;       //!< Buckets with list iterators.
            list_type       m_list;         //!< List with elements.
        };
    }


    template <class Allocator, class NodeType>
    class hash_node_destructor
    {
        using allocator_type = typename AZStd::allocator_traits<Allocator>::template rebind_alloc<NodeType>;
        using allocator_traits = AZStd::allocator_traits<allocator_type>;

    public:

        explicit hash_node_destructor(allocator_type& allocator)
            : m_allocator(allocator)
        {}

        void operator()(NodeType* nodePtr)
        {
            if (nodePtr)
            {
                allocator_traits::destroy(m_allocator, nodePtr);
                allocator_traits::deallocate(m_allocator, nodePtr, sizeof(NodeType), alignof(NodeType));
            }
        }

    private:
        hash_node_destructor& operator=(const hash_node_destructor&) = delete;
        hash_node_destructor(const hash_node_destructor&) = delete;

        allocator_type& m_allocator;
    };

    /**
     * Hash table is internal container used as a base class for all unordered associative containers.
     * It provides functionality for the unordered container in \ref CTR1. (6.3.4). In addition we introduce the following \ref HashExtensions "extensions".
     *
     * Traits should have the following members
     * typedef xxx  key_type;
     * typedef xxx  key_equal;
     * typedef xxx  hasher;
     * typedef xxx  value_type;
     * typedef xxx  allocator_type;
     * enum
     * {
     *   max_load_factor = xxx,   // What should the max load factor before we grow the map. Load factor is the average num of elements per bucket.
     *   min_buckets = xxx,       // Min num of buckets to be allocated.
     *   has_multi_elements = true or false,
     *   is_dynamic = true or false,  // true if we have fixed container. If we do so we will need to se fixed_num_buckets and fixed_num_elements.
     *   fixed_num_buckets = xxx,   // Number of buckets to pre-allocate. For hashing purposes it will be good to be a prime number. We we get better bucket distribution.
     *                              // It should be aprox. 1/3 - 1/4 (max_load_factor 3 or 4) of the number of elements, otherwise we will have too much liear searches.
     *   fixed_num_elements = xxx,  // Number of elements to pre-allocate.
     * }
     *
     * static inline key_type key_from_value(const value_type& value);
     *
     * \todo hash_table_compact using forward_list container, no reverse iterator, slower insert and erase (up to O(n)) but fast find. Using less memory.
     */
    template<class Traits>
    class hash_table
    {
        typedef hash_table<Traits>  this_type;
        typedef AZStd::integral_constant<bool, Traits::is_dynamic>   is_dynamic;
        typedef Internal::hash_table_storage<Traits>                storage_type;
        friend class Internal::hash_table_storage<Traits>;
    public:
        typedef Traits                          traits_type;

        typedef typename Traits::key_type       key_type;
        typedef typename Traits::key_equal         key_equal;
        typedef typename Traits::hasher         hasher;

        typedef typename Traits::allocator_type                 allocator_type;
        typedef typename storage_type::list_type                list_type;

        typedef typename list_type::size_type                   size_type;
        typedef typename list_type::difference_type             difference_type;
        typedef typename list_type::pointer                     pointer;
        typedef typename list_type::const_pointer               const_pointer;
        typedef typename list_type::reference                   reference;
        typedef typename list_type::const_reference             const_reference;

        typedef typename list_type::iterator                    iterator;
        typedef typename list_type::const_iterator              const_iterator;

        typedef typename list_type::reverse_iterator            reverse_iterator;
        typedef typename list_type::const_reverse_iterator      const_reverse_iterator;

        typedef typename list_type::value_type                  value_type;

        typedef iterator        local_iterator;
        typedef const_iterator  const_local_iterator;

        typedef typename storage_type::vector_value_type        vector_value_type;
        typedef typename storage_type::vector_type              vector_type;

        typedef AZStd::pair<iterator, bool>                     pair_iter_bool;
        typedef AZStd::pair<iterator, iterator>                 pair_iter_iter;
        typedef AZStd::pair<const_iterator, const_iterator>     pair_citer_citer;

        typedef typename list_type::node_type                   list_node_type;
        typedef typename vector_type::node_type                 vector_node_type;
        typedef hash_node_destructor<allocator_type, list_node_type> node_deleter;

        AZ_FORCE_INLINE hash_table(const hasher& hash, const key_equal& keyEqual, const allocator_type& alloc = allocator_type())
            : m_data(alloc)
            , m_keyEqual(keyEqual)
            , m_hasher(hash)
        {}


        AZ_FORCE_INLINE hash_table(const value_type* first, const value_type* last, const hasher& hash, const key_equal& keyEqual, const allocator_type& alloc)
            : m_data(alloc)
            , m_keyEqual(keyEqual)
            , m_hasher(hash)
        {
            insert(first, last);
        }

        hash_table(const hash_table& rhs)
            : m_data(rhs.m_data)
            , m_keyEqual(rhs.m_keyEqual)
            , m_hasher(rhs.m_hasher)
        {
            copy(rhs);
        }
        hash_table(const hash_table& rhs, const type_identity_t<allocator_type>& alloc)
            : m_data(rhs.m_data, alloc)
            , m_keyEqual(rhs.m_keyEqual)
            , m_hasher(rhs.m_hasher)
        {
            copy(rhs);
        }

        hash_table(hash_table&& rhs)
            : m_data(rhs.m_data)
            , m_keyEqual(AZStd::move(rhs.m_keyEqual))
            , m_hasher(AZStd::move(rhs.m_hasher))
        {
            assign_rv(AZStd::move(rhs));
        }
        hash_table(hash_table&& rhs, const type_identity_t<allocator_type>& alloc)
            : m_data(alloc)
            , m_keyEqual(AZStd::move(rhs.m_keyEqual))
            , m_hasher(AZStd::move(rhs.m_hasher))
        {
            assign_rv(AZStd::move(rhs));
        }

        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                assign_rv(AZStd::forward<this_type>(rhs));
            }
            return *this;
        }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            if (this != &rhs)
            {
                copy(rhs);
            }
            return *this;
        }

        AZ_FORCE_INLINE iterator            begin()         { return m_data.m_list.begin(); }
        AZ_FORCE_INLINE const_iterator      begin() const   { return m_data.m_list.begin(); }
        AZ_FORCE_INLINE iterator            end()           { return m_data.m_list.end(); }
        AZ_FORCE_INLINE const_iterator      end() const     { return m_data.m_list.end(); }

        AZ_FORCE_INLINE local_iterator          begin(size_type bucket)
        {
            AZSTD_CONTAINER_ASSERT(bucket < m_data.get_num_buckets(), "AZStd::hash_table::begin() - invalid bucket index!");
            return m_data.buckets()[bucket].second;
        }
        AZ_FORCE_INLINE const_local_iterator    begin(size_type bucket) const
        {
            AZSTD_CONTAINER_ASSERT(bucket < m_data.get_num_buckets(), "AZStd::hash_table::begin() - invalid bucket index!");
            return m_data.buckets()[bucket].second;
        }
        AZ_FORCE_INLINE local_iterator          end(size_type bucket)
        {
            AZSTD_CONTAINER_ASSERT(bucket < m_data.get_num_buckets(), "AZStd::hash_table::begin() - invalid bucket index!");
            vector_value_type& b = m_data.buckets()[bucket];
            return AZStd::next(b.second, b.first);
        }
        AZ_FORCE_INLINE const_local_iterator    end(size_type bucket) const
        {
            AZSTD_CONTAINER_ASSERT(bucket < m_data.get_num_buckets(), "AZStd::hash_table::begin() - invalid bucket index!");
            const vector_value_type& b = m_data.buckets()[bucket];
            return AZStd::next(b.second, b.first);
        }

        // START UNIQUE
        AZ_FORCE_INLINE reverse_iterator        rbegin()        { return m_data.m_list.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const  { return m_data.m_list.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator        rend()          { return m_data.m_list.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const    { return m_data.m_list.rend(); }
        // END UNIQUE

        AZ_FORCE_INLINE size_type       size() const                { return m_data.m_list.size(); }
        AZ_FORCE_INLINE size_type       max_size() const            { return m_data.m_list.max_size(); }
        AZ_FORCE_INLINE bool            empty() const               { return m_data.m_list.empty(); }
        AZ_FORCE_INLINE key_equal       key_eq() const           { return m_keyEqual; }
        AZ_FORCE_INLINE hasher          get_hasher() const          { return m_hasher; }
        AZ_FORCE_INLINE size_type       bucket_count() const        { return m_data.get_num_buckets(); }
        AZ_FORCE_INLINE size_type       max_bucket_count() const    { return m_data.m_vector.max_size(); }
        AZ_FORCE_INLINE size_type       bucket(const key_type& keyValue) const { return bucket_from_hash(m_hasher(keyValue)); }
        AZ_INLINE size_type             bucket_size(size_type bucket) const { return m_data.buckets()[bucket].first; }

        AZ_FORCE_INLINE float           load_factor() const         {   return ((float)m_data.m_list.size() / (float)m_data.get_num_buckets()); }
        AZ_FORCE_INLINE float           max_load_factor() const     {   return m_data.max_load_factor();    }
        AZ_FORCE_INLINE void            max_load_factor(float newMaxLoadFactor) { m_data.max_load_factor(newMaxLoadFactor); m_data.rehash_if_needed(this); }
        AZ_FORCE_INLINE pair_iter_bool  insert(const value_type& value) { return insert_impl(value); } // try to insert node with value value
        AZ_FORCE_INLINE iterator        insert(const_iterator, const value_type& value)
        {
            // try to insert node with value value, ignore hint
            // This is a demonstration how customizable is insert_from, you can control the entire process when needed for speed.
            return insert_from(value, ConvertFromValue(), m_hasher, m_keyEqual).first;
        }
        AZ_FORCE_INLINE pair_iter_bool  insert(value_type&& value)
        {
            return insert_impl(AZStd::forward<value_type>(value));
        }
        AZ_FORCE_INLINE iterator  insert(const_iterator, value_type&& value)
        {
            return insert_impl(AZStd::forward<value_type>(value)).first;
        }

        template <typename... Args>
        AZ_FORCE_INLINE pair_iter_bool emplace(Args&&... arguments)
        {
            return insert_impl_emplace(AZStd::forward<Args>(arguments)...);
        }

        AZ_FORCE_INLINE void insert(std::initializer_list<value_type> list)
        {
            insert(list.begin(), list.end());
        }

        // START UNIQUE
        template<class Iterator>
        auto insert(Iterator first, Iterator last)
            -> enable_if_t<input_iterator<Iterator> && !is_convertible_v<Iterator, size_type>>
        {
            // insert [first, last) one at a time
            auto inserter = [this](Iterator it, auto&& valueKey)
            {
                size_type bucketIndex = bucket_from_hash(m_hasher(valueKey));
                vector_value_type& bucket = m_data.buckets()[bucketIndex];
                size_type& numElements = bucket.first;

                iterator iter = bucket.second;
                if (numElements == 0)
                {
                    m_data.m_list.push_front(*it);
                    bucket.second = m_data.m_list.begin();
                }
                else
                {
                    if (!find_insert_position(AZStd::forward<decltype(valueKey)>(valueKey), m_keyEqual, iter, numElements, AZStd::integral_constant<bool, Traits::has_multi_elements>()))
                    {
                        return;
                    }
                    m_data.m_list.insert(iter, *it);
                }
                ++numElements;
            };
            for (; first != last; ++first)
            {
                inserter(first, Traits::key_from_value(*first));
            }

            m_data.rehash_if_needed(this);
        }
        template<class R>
        auto insert_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>>
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                insert(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                insert(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }


        //! Returns an insert_return_type with the members initialized as follows: if nh is empty, inserted is false, position is end(), and node is empty.
        //! Otherwise if the insertion took place, inserted is true, position points to the inserted element, and node is empty.
        //! If the insertion failed, inserted is false, node has the previous value of nh, and position points to an element with a key equivalent to nh.key().
        template <class InsertReturnType, class NodeHandle>
        InsertReturnType node_handle_insert(NodeHandle&& nodeHandle);
        template <class NodeHandle>
        auto node_handle_insert(const_iterator hint, NodeHandle&& nodeHandle) -> iterator;

        //! Searches for an element which matches the value of key and extracts it from the hash_table
        //! @return A NodeHandle which can be used to insert the an element between unique and non-unique containers of the same type
        //! i.e a NodeHandle from an unordered_map can be used to insert a node into an unordered_multimap, but not a std::map
        template <class NodeHandle>
        NodeHandle node_handle_extract(const key_type& key);
        //! Finds an element within the hash_table that is represented by the supplied iterator and extracts it
        //! @return A NodeHandle which can be used to insert the an element between unique and non-unique containers of the same type
        template <class NodeHandle>
        NodeHandle node_handle_extract(const_iterator it);
    private:
        //! Removes an element from the hash_table without deleting it. This allows the element to be extracted and moved to another
        //! hash_table container with the same key, value and allocator_type
        auto remove(const_iterator it)->list_node_type*;
    public:

        AZ_FORCE_INLINE void            reserve(size_type numBucketsMin) { rehash((size_type)::ceilf((float)numBucketsMin / max_load_factor())); }
        AZ_FORCE_INLINE void            rehash(size_type numBucketsMin) { m_data.rehash(this, numBucketsMin); }

        iterator        erase(const_iterator erasePos)
        {
            // erase element at erasePos
            vector_value_type& bucket = m_data.buckets()[bucket_from_hash(m_hasher(Traits::key_from_value(*erasePos)))];
            AZ_Assert(bucket.first >= 1, "AZStd::hash_table::erase - This bucket has not elements!");
            --bucket.first;
            if (erasePos == bucket.second)
            {
                ++bucket.second;
            }
            return m_data.m_list.erase(erasePos);
        }

        size_type       erase(const key_type& keyValue)
        {
            // erase and count all that match keyValue
            size_type numErased = 0;

            // erase element at erasePos
            size_type bucketIndex = bucket_from_hash(m_hasher(keyValue));
            vector_value_type& bucket = m_data.buckets()[bucketIndex];

            iterator eraseFirst = bucket.second, eraseLast;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++eraseFirst)
            {
                if (m_keyEqual(Traits::key_from_value(*eraseFirst), keyValue))
                {
                    size_type iNext = i + 1;
                    for (eraseLast = AZStd::next(eraseFirst); iNext < numElements && m_keyEqual(Traits::key_from_value(*eraseLast), keyValue); ++eraseLast, ++iNext)
                    {
                    }
                    numErased = iNext - i;
                    break;
                }
            }

            if (numErased > 0)
            {
                bucket.first -= numErased;
                if (eraseFirst == bucket.second)
                {
                    AZStd::advance(bucket.second, numErased);
                }

                m_data.m_list.erase(eraseFirst, eraseLast);
            }

            return numErased;
        }

        // END UNIQUE
        iterator        erase(const_iterator first, const_iterator last)
        {
            // erase [first, last)
            if (first == m_data.m_list.begin() && last == m_data.m_list.end())
            {
                // erase all
                clear();
                return m_data.m_list.begin();
            }
            else
            {
                // partial erase, one at a time
                while (first != last)
                {
                    first = erase(first);
                }
                return first.base();
            }
        }

        AZ_FORCE_INLINE void            clear()
        {
            // erase all
            m_data.m_list.clear();
            m_data.m_vector.clear();

            m_data.init_buckets();
        }

        template<class ComparableToKey>
        auto find(const ComparableToKey& key)
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            return lower_bound(key);
        }

        template<class ComparableToKey>
        auto find(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return lower_bound(key);
        }

        template<class ComparableToKey>
        auto contains(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, bool>
        {
            return find(key) != end();
        }

        template<class ComparableToKey>
        auto count(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, size_type>
        {   // count all elements that match keyValue
            pair_citer_citer pair = equal_range(key);
            size_type num = AZStd::distance(pair.first, pair.second);
            return num;
        }

        // START UNIQUE
        template<class ComparableToKey>
        auto lower_bound(const ComparableToKey& key)
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            vector_value_type& bucket = m_data.buckets()[bucket_from_hash(m_hasher(key))];
            iterator iter = bucket.second;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++iter)
            {
                if (m_keyEqual(Traits::key_from_value(*iter), key))
                {
                    return iter;
                }
            }
            return m_data.m_list.end();
        }

        template<class ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return const_iterator(const_cast<hash_table*>(this)->lower_bound(key));
        }

        template<class ComparableToKey>
        auto upper_bound(const ComparableToKey& key)
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            vector_value_type& bucket = m_data.buckets()[bucket_from_hash(m_hasher(key))];
            iterator iter = bucket.second;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++iter)
            {
                if (m_keyEqual(Traits::key_from_value(*iter), key))
                {
                    ++i;
                    ++iter;
                    for (; (i < numElements) && m_keyEqual(Traits::key_from_value(*iter), key); ++iter, ++i)
                    {
                    }
                    return iter;
                }
            }
            return m_data.m_list.end();
        }

        template<class ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return const_iterator(const_cast<hash_table*>(this)->upper_bound(key));
        }

        template<class ComparableToKey>
        auto equal_range(const ComparableToKey& key)
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<iterator, iterator>>
        {
            vector_value_type& bucket = m_data.buckets()[bucket_from_hash(m_hasher(key))];
            iterator iter = bucket.second;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++iter)
            {
                if (m_keyEqual(Traits::key_from_value(*iter), key))
                {
                    iterator first = iter;
                    for (; i < numElements; ++i, ++iter)
                    {
                        if (!m_keyEqual(Traits::key_from_value(*iter), key))
                        {
                            break;
                        }
                    }

                    if (first == iter)
                    {
                        break;
                    }

                    return { first, iter };
                }
            }
            return { m_data.m_list.end(), m_data.m_list.end() };
        }

        template<class ComparableToKey>
        auto equal_range(const ComparableToKey& key) const
            -> enable_if_t<(Internal::is_transparent<key_equal, ComparableToKey>::value && Internal::is_transparent<hasher, ComparableToKey>::value) || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<const_iterator, const_iterator>>
        {
            AZStd::pair<iterator, iterator> non_const_range = const_cast<hash_table*>(this)->equal_range(key);
            return { const_iterator(non_const_range.first), const_iterator(non_const_range.second) };
        }

        // END UNIQUE
        AZ_INLINE void swap(this_type& rhs)
        {
            if (this == &rhs)
            {
                return;
            }
            m_data.swap(rhs.m_data);
        }

        /**
        * \anchor HashExtensions
        * \name Extensions
        * @{
        */
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_data.m_allocator; }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_data.m_allocator; }
        void set_allocator(const allocator_type& allocator)             { m_data.set_allocator(allocator); }
        // START UNIQUE
        /**
        * This is similar to lazy_find in this paper \ref C14IDEAS. The idea is to be able to customize the search.
        * For non key_type objects.
        */
        template<class ComparableToKey, class Hasher, class KeyEqual>
        iterator        find_as(const ComparableToKey& keyCmp, const Hasher& hash, const KeyEqual& keyEq)
        {
            vector_value_type& bucket = m_data.buckets()[bucket_from_hash(hash(keyCmp))];
            iterator iter = bucket.second;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++iter)
            {
                if (keyEq(keyCmp, Traits::key_from_value(*iter)))
                {
                    return iter;
                }
            }
            return m_data.m_list.end();
        }
        template<class ComparableToKey, class Hasher, class KeyEqual>
        const_iterator  find_as(const ComparableToKey& keyCmp, const Hasher& hash, const KeyEqual& keyEq) const
        {
            const vector_value_type& bucket = m_data.buckets()[bucket_from_hash(hash(keyCmp))];
            iterator iter = bucket.second;
            size_type numElements = bucket.first;
            for (size_type i = 0; i < numElements; ++i, ++iter)
            {
                if (keyEq(keyCmp, Traits::key_from_value(*iter)))
                {
                    return iter;
                }
            }
            return m_data.m_list.end();
        }

        /**
        * Inserts from a value and converter object.
        * Converter object has the following interface
        * struct MyConverter
        * {
        *   typedef Map::key_type or CompareableToKeyType key_type;
        *   const key_type& to_key(const& MyValue) const;
        *   Map::value_type to_value(const& MyValue) const;
        * }
        * This allow us to convert any "userValue" (U parameter) to a key, check if we can add it to the list,
        * if so, we call to_value function to create (to_key(userValue), to_value(userValue)) and add it to the hash table.
        * This is similar to lazy_insert in this paper \ref C14IDEAS. There is an example why it's beneficial, you can check
        * \ref AZStdExamples. The main idea is that you don't to create unnecessary expensive temporaries.
        */
        template<class U, class Converter, class Hasher, class KeyEqual>
        pair_iter_bool insert_from(const U& userValue, const Converter& convert, const Hasher& hash, const KeyEqual& keyEq)
        {
            (void)convert;
            const typename Converter::key_type& valueKey = convert.to_key(userValue);
            size_type bucketIndex = bucket_from_hash(hash(valueKey));
            vector_value_type& bucket = m_data.buckets()[bucketIndex];
            size_type& numElements = bucket.first;

            iterator insertPos, iter = bucket.second;
            if (numElements == 0)
            {
                // \note \todo For this to work properly we should add insert function to the list with a converter from a user data type.
                // Otherwise we still have one unnecessary copy.
                //insertPos = m_list.insert_after(find_previous(m_list, m_data.buckets(), iter,bucketIndex),convert.to_value(userValue));
                m_data.m_list.push_front(convert.to_value(userValue));
                insertPos = m_data.m_list.begin();
                bucket.second = insertPos;
            }
            else
            {
                if (!find_insert_position(valueKey, keyEq, iter, numElements, AZStd::integral_constant<bool, Traits::has_multi_elements>()))
                {
                    // discard new list element and return existing
                    return (pair_iter_bool(iter, false));
                }

                // \note \todo For this to work properly we should add insert function to the list with a converter from a user data type.
                // Otherwise we still have one unnecessary copy.
                insertPos = m_data.m_list.insert(iter, convert.to_value(userValue));
            }
            ++numElements;

            m_data.rehash_if_needed(this);

            return (pair_iter_bool(insertPos, true));   // return iterator for new element
        }

        // END UNIQUE

        bool            validate() const    { return m_data.m_list.validate() && m_data.m_vector.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        int             validate_iterator(const iterator& iter) const   { return m_data.m_list.validate_iterator(iter); }
        int             validate_iterator(const const_iterator& iter) const { return m_data.m_list.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        void                leak_and_reset()
        {
            m_data.m_list.leak_and_reset();
            m_data.m_vector.leak_and_reset();

            m_data.init_buckets();
        }
        /// @}

    protected:
        AZ_FORCE_INLINE size_type bucket_from_hash(const size_type key) const   { return key % m_data.get_num_buckets(); }

        void copy(const this_type& rhs)
        {
            // copy entire hash table
            m_data.pre_copy(rhs.m_data);

            for (size_type iBucket = 0; iBucket < rhs.m_data.get_num_buckets(); ++iBucket)
            {
                const vector_value_type& srcBucket = rhs.m_data.buckets()[iBucket];
                if (srcBucket.first > 0)
                {
                    vector_value_type& bucket = m_data.buckets()[iBucket];
                    // copy the elements
                    m_data.m_list.insert(m_data.m_list.begin(), srcBucket.second, AZStd::next(srcBucket.second, srcBucket.first));
                    bucket.second = m_data.m_list.begin();
                    bucket.first = srcBucket.first;
                }
            }
        }

        void assign_rv(this_type&& rhs)
        {
            rhs.swap(*this);
            rhs.clear();
        }

        // find_insert_position sets insertIter to where the element should be inserted
        // and returns true if the element should be inserted, otherwise false
        template<class ComparableToKey, class KeyEq>
        bool find_insert_position(const ComparableToKey& keyCmp, const KeyEq& keyEq, iterator& insertIter, size_type numElements, const true_type& /* is multi elements */)
        {
            for (size_type i = 0; i < numElements; ++i, ++insertIter)
            {
                if (keyEq(keyCmp, Traits::key_from_value(*insertIter)))
                {
                    ++insertIter;
                    break;
                }
            }

            // always return true since multi elements (like multiset) allow repeated elements
            return true;
        }

        template<class ComparableToKey, class KeyEq>
        bool find_insert_position(const ComparableToKey& keyCmp, const KeyEq& keyEq, iterator& insertIter, size_type numElements, const false_type& /* !is multi elements */)
        {
            for (size_type i = 0; i < numElements; ++i, ++insertIter)
            {
                if (keyEq(keyCmp, Traits::key_from_value(*insertIter)))
                {
                    // Element already exists, it shouldn't be inserted as we don't allow more than one repeated element for this specialization
                    return false;
                }
            }

            return true;
        }

        template <typename T>
        pair_iter_bool insert_impl(T&& value)
        {
            // try to insert (possibly existing) node with value value
            const key_type& valueKey = Traits::key_from_value(value);
            size_type bucketIndex = bucket_from_hash(m_hasher(valueKey));
            vector_value_type& bucket = m_data.buckets()[bucketIndex];
            size_type& numElements = bucket.first;

            iterator insertPos, iter = bucket.second;
            if (numElements == 0)
            {
                m_data.m_list.push_front(AZStd::forward<T>(value));
                insertPos = m_data.m_list.begin();
                bucket.second = insertPos;
            }
            else
            {
                if (!find_insert_position(valueKey, m_keyEqual, iter, numElements, AZStd::integral_constant<bool, Traits::has_multi_elements>()))
                {
                    // discard new list element and return existing
                    return (pair_iter_bool(iter, false));
                }

                insertPos = m_data.m_list.insert(iter, AZStd::forward<T>(value));
            }
            ++numElements;

            m_data.rehash_if_needed(this);

            return (pair_iter_bool(insertPos, true));   // return iterator for new element
        }

        template <typename... Args>
        pair_iter_bool insert_impl_emplace(Args&&... arguments)
        {
            // Insert new element into the list at the beginning, and move it if appropriate
            m_data.m_list.emplace_front(AZStd::forward<Args>(arguments)...);
            iterator insertPos = m_data.m_list.begin();

            // try to insert (possibly existing) node with value value
            const key_type& valueKey = Traits::key_from_value(*insertPos);
            size_type bucketIndex = bucket_from_hash(m_hasher(valueKey));
            vector_value_type& bucket = m_data.buckets()[bucketIndex];
            size_type& numElements = bucket.first;

            if (numElements == 0)
            {
                bucket.second = insertPos;
            }
            else
            {
                iterator iter = bucket.second;
                if (!find_insert_position(valueKey, m_keyEqual, iter, numElements, AZStd::integral_constant<bool, Traits::has_multi_elements>()))
                {
                    // discard new list element and return existing
                    m_data.m_list.pop_front();
                    return (pair_iter_bool(iter, false));
                }

                m_data.m_list.splice(iter, m_data.m_list, insertPos);
                insertPos = --iter; // decrement iterator so that it points at the new element, not the one after it (splice inserts before iter)
            }
            ++numElements;

            m_data.rehash_if_needed(this);

            return (pair_iter_bool(insertPos, true));   // return iterator for new element
        }

        template <typename ComparableToKey, typename... Args>
        pair_iter_bool try_emplace_transparent(ComparableToKey&& key, Args&&... arguments);

        template <typename ComparableToKey, typename... Args>
        iterator try_emplace_transparent(const_iterator hint, ComparableToKey&& key, Args&&... arguments);

        template <typename ComparableToKey, typename MappedType>
        pair_iter_bool insert_or_assign_transparent(ComparableToKey&& key, MappedType&& value);

        template <typename ComparableToKey, typename MappedType>
        iterator insert_or_assign_transparent(const_iterator hint, ComparableToKey&& key, MappedType&& value);

        storage_type    m_data;
        key_equal          m_keyEqual;
        hasher          m_hasher;

        /**
        * This struct is used to bind the value type to the insert_from function.
        */
        struct ConvertFromValue
        {
            typedef typename Traits::key_type       key_type;

            static AZ_FORCE_INLINE const key_type&      to_key(const value_type& value)     { return Traits::key_from_value(value); }
            static AZ_FORCE_INLINE const value_type&    to_value(const value_type& value)   { return value; }
        };
    };


    template <class Traits>
    template <class InsertReturnType, class NodeHandle>
    InsertReturnType hash_table<Traits>::node_handle_insert(NodeHandle&& nodeHandle)
    {
        if (nodeHandle.empty())
        {
            return { end(), false, NodeHandle{} };
        }

        const key_type& valueKey = Traits::key_from_value(nodeHandle.m_node->m_value);
        size_type bucketIndex = bucket_from_hash(m_hasher(valueKey));
        vector_value_type& bucket = m_data.buckets()[bucketIndex];
        size_type& numElements = bucket.first;

        iterator insertPos, iter = bucket.second;
        if (numElements == 0)
        {
            insertPos = m_data.m_list.relink(m_data.m_list.begin(), nodeHandle.m_node);
            bucket.second = insertPos;
        }
        else
        {
            if (!find_insert_position(valueKey, m_keyEqual, iter, numElements, AZStd::bool_constant<Traits::has_multi_elements>()))
            {
                // The element has been found so return an iterator to it with the inserted flag set to false and the supplied node handle forwarded back to the caller
                return { iter, false, AZStd::move(nodeHandle) };
            }

            insertPos = m_data.m_list.relink(iter, nodeHandle.m_node);
            // Clear the supplied node handle now that the node has been relinked into a list

        }
        nodeHandle.release_node();
        ++numElements;

        m_data.rehash_if_needed(this);

        // The insertion has succeeded in this case so the node handle is empty
        return { insertPos, true, NodeHandle{} };
    }

    template <class Traits>
    template <class NodeHandle>
    inline auto hash_table<Traits>::node_handle_insert(const_iterator, NodeHandle&& nodeHandle) -> iterator
    {
        return node_handle_insert<insert_return_type<iterator, NodeHandle>>(AZStd::move(nodeHandle)).position;
    }

    template <class Traits>
    template <class NodeHandle>
    inline NodeHandle hash_table<Traits>::node_handle_extract(const key_type& key)
    {
        iterator it = find(key);
        if (it == end())
        {
            return {};
        }
        return node_handle_extract<NodeHandle>(it);
    }

    template <class Traits>
    template <class NodeHandle>
    inline NodeHandle hash_table<Traits>::node_handle_extract(const_iterator extractPos)
    {
        return NodeHandle(remove(extractPos), get_allocator());
    }

    template <class Traits>
    inline auto hash_table<Traits>::remove(const_iterator removePos) -> list_node_type*
    {
        // erase element at erasePos
        vector_value_type& bucket = m_data.buckets()[bucket_from_hash(m_hasher(Traits::key_from_value(*removePos)))];
        --bucket.first;
        if (removePos == bucket.second)
        {
            ++bucket.second;
        }

        return m_data.m_list.unlink(removePos);
    }

    template <class Traits>
    template <typename ComparableToKey, typename... Args>
    inline auto hash_table<Traits>::try_emplace_transparent(ComparableToKey&& key, Args&&... arguments) -> pair_iter_bool
    {
        // Check if the key has a corresponding node in the container
        if (iterator findIter = find(key); findIter != m_data.m_list.end())
        {
            return { findIter, false };
        }

        size_type bucketIndex = bucket_from_hash(m_hasher(key));
        auto& [numElements, bucketFrontIter] = m_data.buckets()[bucketIndex];

        // advance to the end of the bucket and piecewise construct the arguments at before that iterator
        iterator insertIter = AZStd::next(bucketFrontIter, numElements);

        if (numElements == 0)
        {
            // No elements in the bucket
            // emplace the element at the beginning of the list and update the bucket front iterator to point to it
            insertIter = m_data.m_list.emplace(m_data.m_list.begin(), AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
                AZStd::forward_as_tuple(AZStd::forward<Args>(arguments)...));
            bucketFrontIter = insertIter;
        }
        else
        {
            // The returned iterator from list::emplace is pointing at the newly inserted element
            insertIter = m_data.m_list.emplace(insertIter, AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
                AZStd::forward_as_tuple(AZStd::forward<Args>(arguments)...));
        }

        // Update the number of elements in the bucket using the numElements reference variable
        ++numElements;

        m_data.rehash_if_needed(this);

        return { insertIter, true };
    }

    template <class Traits>
    template <typename ComparableToKey, typename... Args>
    inline auto hash_table<Traits>::try_emplace_transparent(const_iterator, ComparableToKey&& key, Args&&... arguments) -> iterator
    {
        return try_emplace_transparent(AZStd::forward<ComparableToKey>(key), AZStd::forward<Args>(arguments)...).first;
    }

    template <class Traits>
    template <typename ComparableToKey, typename MappedType>
    inline auto hash_table<Traits>::insert_or_assign_transparent(ComparableToKey&& key, MappedType&& value) -> pair_iter_bool
    {
        // Check if the key has a corresponding node in the container
        if (iterator findIter = find(key); findIter != m_data.m_list.end())
        {
            // Update the mapped element if the key has been found
            findIter->second = AZStd::forward<MappedType>(value);
            return { findIter, false };
        }

        size_type bucketIndex = bucket_from_hash(m_hasher(key));
        auto& [numElements, bucketFrontIter] = m_data.buckets()[bucketIndex];

        // advance to the end of the bucket and piecewise construct the arguments at before that iterator
        iterator insertIter = AZStd::next(bucketFrontIter, numElements);

        if (numElements == 0)
        {
            // No elements in the bucket
            // emplace the element at the beginning of the list and update the bucket front iterator to point to it
            insertIter = m_data.m_list.emplace(m_data.m_list.begin(), AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
                AZStd::forward_as_tuple(AZStd::forward<MappedType>(value)));
            bucketFrontIter = insertIter;
        }
        else
        {
            // The returned iterator from list::emplace is pointing at the newly inserted element
            insertIter = m_data.m_list.emplace(insertIter, AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
                AZStd::forward_as_tuple(AZStd::forward<MappedType>(value)));
        }
        // Update the number of elements in the bucket using the numElements reference variable
        ++numElements;

        m_data.rehash_if_needed(this);

        return { insertIter, true };
    }

    template <class Traits>
    template <typename ComparableToKey, typename MappedType>
    inline auto hash_table<Traits>::insert_or_assign_transparent(const_iterator, ComparableToKey&& key, MappedType&& value) -> iterator
    {
        return insert_or_assign_transparent(AZStd::forward<ComparableToKey>(key), AZStd::forward<MappedType>(value)).first;
    }
}

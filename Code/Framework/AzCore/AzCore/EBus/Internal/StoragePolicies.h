/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Policies.h>
#include <AzCore/EBus/Internal/Debug.h>

#include <AzCore/std/hash_table.h>
#include <AzCore/std/containers/rbtree.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/intrusive_set.h>

namespace AZ
{
    namespace Internal
    {
        /**
         * AddressStoragePolicy is used to determine how to store a mapping of address -> HandlerStorage.
         *
         * \tparam Traits       The traits for the Bus. Used to determin: id type, allocator type, whether to order container, and how to compare ids.
         * \tparam HandlerStorage   The type to use as the value of the map. This will often be a structure containing a list or ordered list of handlers.
         */
        template <typename Traits, typename HandlerStorage, EBusAddressPolicy = Traits::AddressPolicy>
        struct AddressStoragePolicy;
        // Unordered
        template <typename Traits, typename HandlerStorage>
        struct AddressStoragePolicy<Traits, HandlerStorage, EBusAddressPolicy::ById>
        {
        private:
            using IdType = typename Traits::BusIdType;

            struct hash_table_traits
            {
                using key_type = IdType;
                using key_equal = AZStd::equal_to<key_type>;
                using value_type = HandlerStorage;
                using allocator_type = typename Traits::AllocatorType;
                enum
                {
                    max_load_factor = 4,
                    min_buckets = 7,
                    has_multi_elements = false,

                    is_dynamic = true,
                    fixed_num_buckets = 1,
                    fixed_num_elements = 4
                };
                static const key_type& key_from_value(const value_type& value) { return value.m_busId; }
                struct hasher
                {
                    AZStd::size_t operator()(const IdType& id) const
                    {
                        return AZStd::hash<IdType>()(id);
                    }
                };
            };

        public:
            struct StorageType
                : public AZStd::hash_table<hash_table_traits>
            {
                using base_type = AZStd::hash_table<hash_table_traits>;

                StorageType()
                    : base_type(typename base_type::hasher(), typename base_type::key_equal(), typename base_type::allocator_type())
                { }

                // EBus extension: Assert on insert
                template <typename... InputArgs>
                typename base_type::iterator emplace(InputArgs&&... args)
                {
                    auto pair = base_type::emplace(AZStd::forward<InputArgs>(args)...);
                    EBUS_ASSERT(pair.second, "Internal error: Failed to insert");
                    return pair.first;
                }

                // EBus extension: rehash on final erase
                void erase(const IdType& id)
                {
                    base_type::erase(id);
                    if (base_type::empty())
                    {
                        base_type::rehash(0);
                    }
                }
            };
        };
        // Ordered
        template <typename Traits, typename HandlerStorage>
        struct AddressStoragePolicy<Traits, HandlerStorage, EBusAddressPolicy::ByIdAndOrdered>
        {
        private:
            using IdType = typename Traits::BusIdType;

            struct rbtree_traits
            {
                using key_type = IdType;
                using key_equal = typename Traits::BusIdOrderCompare;
                using value_type = HandlerStorage;
                using allocator_type = typename Traits::AllocatorType;
                static const key_type& key_from_value(const value_type& value) { return value.m_busId; }
            };

        public:
            struct StorageType
                : public AZStd::rbtree<rbtree_traits>
            {
                using base_type = AZStd::rbtree<rbtree_traits>;

                // EBus extension: Assert on insert
                template <typename... InputArgs>
                typename StorageType::iterator emplace(InputArgs&&... args)
                {
                    auto pair = base_type::emplace_unique(AZStd::forward<InputArgs>(args)...);
                    EBUS_ASSERT(pair.second, "Internal error: Failed to insert");
                    return pair.first;
                }
            };
        };

        /**
         * Helpers for substituting default compare implementation when BusHandlerCompareDefault is specified.
         */
        template <typename /*Interface*/, typename Traits, typename Comparer = typename Traits::BusHandlerOrderCompare>
        struct HandlerCompare
            : public Comparer
        { };
        template <typename Interface, typename Traits>
        struct HandlerCompare<Interface, Traits, AZ::BusHandlerCompareDefault>
        {
            bool operator()(const Interface* left, const Interface* right) const { return left->Compare(right); }
        };

        /**
         * HandlerStoragePolicy is used to determine how to store a list of handlers. This collection will always be intrusive.
         *
         * \tparam Interface    The interface for the bus. Used as a parameter to the compare function.
         * \tparam Traits       The traits for the Bus. Used to determin: whether to order container, and how to compare handlers.
         * \tparam Handler      The handler type. This will be used as the value of the container. This type is expected to inherit from HandlerStorageNode.
         *                      MUST NOT BE A POINTER TYPE.
         */
        template <typename Interface, typename Traits, typename Handler, EBusHandlerPolicy = Traits::HandlerPolicy>
        struct HandlerStoragePolicy;
        // Unordered
        template <typename Interface, typename Traits, typename Handler>
        struct HandlerStoragePolicy<Interface, Traits, Handler, EBusHandlerPolicy::Multiple>
        {
        public:
            struct StorageType
                : public AZStd::intrusive_list<Handler, AZStd::list_base_hook<Handler>>
            {
                using base_type = AZStd::intrusive_list<Handler, AZStd::list_base_hook<Handler>>;

                void insert(Handler& elem)
                {
                    base_type::push_front(elem);
                }
            };
        };
        // Ordered
        template <typename Interface, typename Traits, typename Handler>
        struct HandlerStoragePolicy<Interface, Traits, Handler, EBusHandlerPolicy::MultipleAndOrdered>
        {
        private:
            using Compare = HandlerCompare<Interface, Traits>;

        public:
            using StorageType = AZStd::intrusive_multiset<Handler, AZStd::intrusive_multiset_base_hook<Handler>, Compare>;
        };

        // Param Handler to HandlerStoragePolicy is expected to inherit from this type.
        template <typename Handler, EBusHandlerPolicy>
        struct HandlerStorageNode
        {
        };
        template <typename Handler>
        struct HandlerStorageNode<Handler, EBusHandlerPolicy::Multiple>
            : public AZStd::intrusive_list_node<Handler>
        {
        };
        template <typename Handler>
        struct HandlerStorageNode<Handler, EBusHandlerPolicy::MultipleAndOrdered>
            : public AZStd::intrusive_multiset_node<Handler>
        {
        };
    } // namespace Internal
} // namespace AZ

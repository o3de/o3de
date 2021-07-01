/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/std/allocator_traits.h>

namespace AZStd
{
    template <class Traits>
    class rbtree;
    template <class Traits>
    class hash_table;

    inline namespace AssociativeInternal
    {
        template <typename ValueType, typename AllocatorType, typename NodeType, typename NodeDeleterType>
        struct set_node_traits
        {
            using value_type = ValueType;
            using allocator_type = AllocatorType;
            using node_type = NodeType;
            using node_deleter_type = NodeDeleterType;
        };

        template <typename KeyType, typename MappedType, typename AllocatorType, typename NodeType, typename NodeDeleterType>
        struct map_node_traits
        {
            using key_type = KeyType;
            using mapped_type = MappedType;
            using allocator_type = AllocatorType;
            using node_type = NodeType;
            using node_deleter_type = NodeDeleterType;
        };

        template <class Iterator, class NodeType>
        struct insert_return_type
        {
            Iterator position;
            bool inserted;
            NodeType node;
        };

        template <typename NodeTraits, template <typename, typename> class MapOrSetNodeHandleBase>
        class node_handle
            : public MapOrSetNodeHandleBase<node_handle<NodeTraits, MapOrSetNodeHandleBase>, NodeTraits>
        {
        public:
            using allocator_type = typename NodeTraits::allocator_type;

        private:
            using allocator_traits = AZStd::allocator_traits<allocator_type>;
            
            template <class Traits> friend class AZStd::rbtree;
            template <class Traits> friend class AZStd::hash_table;
            
            friend MapOrSetNodeHandleBase<node_handle<NodeTraits, MapOrSetNodeHandleBase>, NodeTraits>;

            using node_pointer_type = typename NodeTraits::node_type*;
            using node_allocator_type = typename allocator_traits::template rebind_alloc<typename NodeTraits::node_type>;
            using node_deleter_type = typename NodeTraits::node_deleter_type;
        public:

            node_handle() = default;
            node_handle(node_handle&& other)
                : m_node(other.m_node)
                , m_allocator(AZStd::move(other.m_allocator))
            {
                other.release_node();
            }

            node_handle& operator=(node_handle&& other)
            {
                AZSTD_CONTAINER_ASSERT(m_allocator == AZStd::nullopt || allocator_traits::propagate_on_container_move_assignment::value || m_allocator == other.m_allocator,
                    "node_type with incompatible allocator passed to node_type::operator=(node_type&&)");

                destroy_node();
                m_node = other.m_node;

                if (m_allocator == AZStd::nullopt || allocator_traits::propagate_on_container_move_assignment::value)
                {
                    m_allocator = AZStd::move(other.m_allocator);
                }

                other.release_node();

                return *this;
            }

            ~node_handle()
            {
                destroy_node();
            }

            bool empty() const
            {
                return m_node == nullptr;
            }

            explicit operator bool() const
            {
                return m_node != nullptr;
            }

            allocator_type get_allocator() const
            {
                return *m_allocator;
            }

            void swap(node_handle& other)
            {
                using AZStd::swap;
                node_handle temp{ AZStd::move(*this) };
                swap(m_node, other.m_node);
                if (allocator_traits::propagate_on_container_move_assignment::value || m_allocator.empty() || other.empty())
                {
                    swap(m_allocator, other.m_allocator);
                }
            }

            friend void swap(node_handle& lhs, node_handle& rhs);

        private:
            node_handle(node_pointer_type node, const allocator_type& allocator)
                : m_node(node)
                , m_allocator(allocator)
            {
            }

            void release_node()
            {
                m_node = nullptr;
                m_allocator = {};
            }

            void destroy_node()
            {
                if (m_node)
                {
                    node_deleter_type nodeDeleter(*m_allocator);
                    nodeDeleter(m_node);
                    m_node = nullptr;
                }
            }

            node_pointer_type m_node{};
            optional<allocator_type> m_allocator;
        };

        template <typename NodeTraits, template <typename, typename> class MapOrSetNodeHandleBase>
        void swap(node_handle<NodeTraits, MapOrSetNodeHandleBase>& lhs, node_handle<NodeTraits, MapOrSetNodeHandleBase>& rhs)
        {
            lhs.swap(rhs);
        }

        template <typename NodeHandleType, typename NodeTraits>
        struct set_node_base
        {
            using value_type = typename NodeTraits::value_type;

            value_type& value() const
            {
                return static_cast<const NodeHandleType&>(*this).m_node->m_value;
            }

        };

        template <typename NodeHandleType, typename NodeTraits>
        struct map_node_base
        {
            using key_type = typename NodeTraits::key_type;
            using mapped_type = typename NodeTraits::mapped_type;

            key_type& key() const
            {
                return static_cast<const NodeHandleType&>(*this).m_node->m_value.first;
            }
            mapped_type& mapped() const
            {
                return static_cast<const NodeHandleType&>(*this).m_node->m_value.second;
            }
        };

        template <typename NodeTraits>
        using set_node_handle = node_handle<NodeTraits, set_node_base>;

        template <typename NodeTraits>
        using map_node_handle = node_handle<NodeTraits, map_node_base>;
    }
}

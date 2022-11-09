/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>

// Allow us to save up to 25% of the node overhead. And yes the code is faster too.
#define AZSTD_RBTREE_USE_COMPRESSED_NODE

namespace AZStd
{
    #define AZSTD_RBTREE_RED    0
    #define AZSTD_RBTREE_BLACK  1

    namespace Internal
    {
        struct rbtree_node_base
        {
            typedef int               color_type;
            typedef rbtree_node_base* BaseNodePtr;

#ifdef AZSTD_RBTREE_USE_COMPRESSED_NODE
            BaseNodePtr   m_parentColor;    ///< Compress the color in the least significant bit
#else
            color_type    m_color;
            BaseNodePtr   m_parent;
#endif
            BaseNodePtr   m_left;
            BaseNodePtr   m_right;

#ifdef AZSTD_RBTREE_USE_COMPRESSED_NODE
            AZ_FORCE_INLINE color_type get_color() const
            {
                return static_cast<color_type>(reinterpret_cast<AZStd::size_t>(m_parentColor) & 1);
            }
            AZ_FORCE_INLINE void set_color(color_type color)
            {
                AZStd::size_t parentColor = reinterpret_cast<AZStd::size_t>(m_parentColor);
                parentColor &= ~(AZStd::size_t)1;
                parentColor |= (AZStd::size_t)color;
                m_parentColor = reinterpret_cast<BaseNodePtr>(parentColor);
            }
            AZ_FORCE_INLINE BaseNodePtr get_parent() const
            {
                AZStd::size_t parentColor = reinterpret_cast<AZStd::size_t>(m_parentColor);
                parentColor &= ~(AZStd::size_t)1;
                return reinterpret_cast<BaseNodePtr>(parentColor);
            }
            AZ_FORCE_INLINE void set_parent(BaseNodePtr parent)
            {
                AZSTD_CONTAINER_ASSERT((reinterpret_cast<AZStd::size_t>(parent) & 1) == 0, "We require nodes to be at least 2 bytes aligned!");
                AZStd::size_t newParentColor = reinterpret_cast<AZStd::size_t>(parent);
                AZStd::size_t parentColor = reinterpret_cast<AZStd::size_t>(m_parentColor);
                parentColor &= 1;
                parentColor |= newParentColor;
                m_parentColor = reinterpret_cast<BaseNodePtr>(parentColor);
            }
            inline void swap_color(rbtree_node_base& rhs)
            {
                color_type col = static_cast<color_type>(reinterpret_cast<AZStd::size_t>(m_parentColor) & 1);
                color_type rhsCol = static_cast<color_type>(reinterpret_cast<AZStd::size_t>(rhs.m_parentColor) & 1);
                col ^= rhsCol;
                rhsCol ^= col;
                col ^= rhsCol;
                AZStd::size_t parentColor = reinterpret_cast<AZStd::size_t>(m_parentColor);
                AZStd::size_t rhsParentColor = reinterpret_cast<AZStd::size_t>(rhs.m_parentColor);
                parentColor &= ~(AZStd::size_t)1;
                rhsParentColor &= ~(AZStd::size_t)1;
                parentColor |= (AZStd::size_t)col;
                rhsParentColor |= (AZStd::size_t)rhsCol;
                m_parentColor = reinterpret_cast<BaseNodePtr>(parentColor);
                rhs.m_parentColor = reinterpret_cast<BaseNodePtr>(rhsParentColor);
            }
#else
            AZ_FORCE_INLINE color_type get_color() const          { return m_color; }
            AZ_FORCE_INLINE void set_color(color_type color)      { m_color = color; }
            AZ_FORCE_INLINE BaseNodePtr get_parent() const        { return m_parent; }
            AZ_FORCE_INLINE void set_parent(BaseNodePtr parent)   { m_parent = parent; }
            AZ_FORCE_INLINE void swap_color(rbtree_node_base& rhs) { AZStd::swap(m_color, rhs.m_color); }
#endif // AZSTD_RBTREE_USE_COMPRESSED_NODE

            AZ_FORCE_INLINE BaseNodePtr minimum()
            {
                BaseNodePtr node = this;
                while (node->m_left != 0)
                {
                    node = node->m_left;
                }
                return node;
            }
            AZ_FORCE_INLINE BaseNodePtr maximum()
            {
                BaseNodePtr node = this;
                while (node->m_right != 0)
                {
                    node = node->m_right;
                }
                return node;
            }
        };

        template <class Value>
        struct rbtree_node
            : public rbtree_node_base
        {
            Value m_value;
        };
    }

    /**
     * Red-Black tree const iterator implementation. SCARY iterator implementation.
     */
    template<class T>
    class rbtree_const_iterator
    {
        enum
        {
            ITERATOR_VERSION = 1
        };

        template<class Traits>
        friend class rbtree;
        typedef rbtree_const_iterator               this_type;
    public:
        typedef AZStd::bidirectional_iterator_tag   iterator_category;
        typedef AZStd::ptrdiff_t                    difference_type;
        typedef T                                   value_type;
        typedef const T*                            pointer;
        typedef const T&                            reference;

        typedef Internal::rbtree_node_base*         base_node_ptr_type;
        typedef const Internal::rbtree_node_base*   const_base_node_ptr_type;

        typedef Internal::rbtree_node<T>            node_type;
        typedef node_type*                          node_ptr_type;
        typedef const node_type*                    const_node_ptr_type;

        AZ_FORCE_INLINE rbtree_const_iterator()
            : m_node(0) {}
        AZ_FORCE_INLINE rbtree_const_iterator(base_node_ptr_type baseNode)
            : m_node(static_cast<node_ptr_type>(baseNode)) {}
        AZ_FORCE_INLINE rbtree_const_iterator(node_ptr_type node)
            : m_node(node) {}
        //copy constructor for iterator and constructor from iterator for const_iterator
        AZ_FORCE_INLINE rbtree_const_iterator(const rbtree_const_iterator& rhs)
            : m_node(rhs.m_node) {}

        AZ_FORCE_INLINE reference operator*() const { return m_node->m_value;  }
        AZ_FORCE_INLINE pointer operator->() const  { return &(m_node->m_value); }
        inline this_type& operator++()
        {
            if (m_node->m_right != 0)
            {
                m_node = static_cast<node_ptr_type>(m_node->m_right->minimum());
            }
            else
            {
                base_node_ptr_type y = m_node->get_parent();
                while (m_node == y->m_right)
                {
                    m_node = static_cast<node_ptr_type>(y);
                    y = y->get_parent();
                }
                // check special case: This is necessary if m_node is the
                // m_head and the tree contains only a single node y. In
                // that case parent, left and right all point to y!
                if (m_node->m_right != y)
                {
                    m_node = static_cast<node_ptr_type>(y);
                }
            }
            return *this;
        }
        AZ_FORCE_INLINE this_type operator++(int)
        {
            this_type tmp = *this;
            ++(*this);
            return tmp;
        }

        inline this_type& operator--()
        {
            if (m_node->get_color() == AZSTD_RBTREE_RED && m_node->get_parent()->get_parent() == m_node)
            {
                m_node = static_cast<node_ptr_type>(m_node->m_right);
            }
            else if (m_node->m_left != 0)
            {
                m_node = static_cast<node_ptr_type>(m_node->m_left->maximum());
            }
            else
            {
                base_node_ptr_type y = m_node->get_parent();
                base_node_ptr_type node = m_node;
                while (node == y->m_left)
                {
                    node = y;
                    y = y->get_parent();
                }
                m_node = static_cast<node_ptr_type>(y);
            }
            return *this;
        }
        AZ_FORCE_INLINE this_type operator--(int)
        {
            this_type tmp = *this;
            --(*this);
            return tmp;
        }

        AZ_FORCE_INLINE bool operator == (this_type rhs) const { return m_node == rhs.m_node; }
        AZ_FORCE_INLINE bool operator != (this_type rhs) const { return m_node != rhs.m_node;  }

        // workaround MSVC C++17 reverse_iterator implementation, where it requires iterators supplied to reverse_iterator
        // to have all the comparison operators and the subscript operator[] implemented due to checking
        // those operators in a no-except block
        // When real C++ concepts are enabled, it properly removes the reverse_iterator comparison operators
        // and subscript operators from the candidate set
        // The workaround is to declare the operator functions, but don't define them.
        bool operator<(this_type rhs) const;
        bool operator>(this_type rhs) const;
        bool operator<=(this_type rhs) const;
        bool operator>=(this_type rhs) const;
        reference operator[](size_t) const;
        rbtree_const_iterator operator+(difference_type) const;
        rbtree_const_iterator& operator+=(difference_type);
        rbtree_const_iterator operator-(difference_type) const;
        rbtree_const_iterator& operator-=(difference_type);

    protected:
        node_ptr_type m_node;
    };

    /**
     * Red-Black tree iterator implementation. SCARY iterator implementation.
     */
    template<class T>
    class rbtree_iterator
        : public rbtree_const_iterator<T>
    {
        typedef rbtree_iterator             this_type;
        typedef rbtree_const_iterator<T>    base_type;
    public:
        typedef T*                          pointer;
        typedef T&                          reference;
        using difference_type = typename base_type::difference_type;

        typedef typename base_type::base_node_ptr_type  base_node_ptr_type;
        typedef typename base_type::node_ptr_type       node_ptr_type;

        AZ_FORCE_INLINE rbtree_iterator() {}
        AZ_FORCE_INLINE rbtree_iterator(base_node_ptr_type baseNode)
            : rbtree_const_iterator<T>(baseNode) {}
        AZ_FORCE_INLINE rbtree_iterator(node_ptr_type node)
            : rbtree_const_iterator<T>(node) {}
        AZ_FORCE_INLINE reference operator*() const { return base_type::m_node->m_value; }
        AZ_FORCE_INLINE pointer operator->() const { return &(base_type::m_node->m_value); }
        AZ_FORCE_INLINE this_type& operator++() { base_type::operator++();  return *this;   }
        AZ_FORCE_INLINE this_type operator++(int)
        {
            this_type temp = *this;
            ++*this;
            return temp;
        }
        AZ_FORCE_INLINE this_type& operator--() { base_type::operator--(); return *this; }
        AZ_FORCE_INLINE this_type operator--(int)
        {
            this_type temp = *this;
            --*this;
            return temp;
        }

        // workaround MSVC C++17 reverse_iterator implementation, where it requires iterators supplied to reverse_iterator
        // to have all the comparison operators and the subscript operator[] implemented due to checking
        // those operators in a no-except block
        // When real C++ concepts are enabled, it properly removes the reverse_iterator comparison operators
        // and subscript operators from the candidate set
        // The workaround is to declare the operator functions, but don't define them.
        reference operator[](size_t) const;
        rbtree_iterator operator+(difference_type) const;
        rbtree_iterator& operator+=(difference_type);
        rbtree_iterator operator-(difference_type) const;
        rbtree_iterator& operator-=(difference_type);
    };

    template <class Allocator, class NodeType>
    class rbtree_node_destructor
    {
        using allocator_type = typename AZStd::allocator_traits<Allocator>::template rebind_alloc<NodeType>;
        using allocator_traits = AZStd::allocator_traits<allocator_type>;

    public:

        explicit rbtree_node_destructor(allocator_type& allocator)
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
        rbtree_node_destructor& operator=(const rbtree_node_destructor&) = delete;
        rbtree_node_destructor(const rbtree_node_destructor&) = delete;

        allocator_type& m_allocator;
    };

    /**
     * Generic red-black tree. Based on the STLport implementation. In addition to all
     * AZStd extensions and requirements, we use compressed node which saves about ~25%
     * of the tree overhead. This is the base container used for AZStd::set,AZStd::multiset,AZStd::map and AZStd::multimap.
     *
     * \ref RedBlackTreeTest for examples.
     *
     * Traits should have the following members
     * typedef xxx  key_type;
     * typedef xxx  key_equal;
     * typedef xxx  value_type;
     * typedef xxx  allocator_type;
     * enum
     * {
     *   has_multi_elements = true or false,
     *   is_dynamic = true or false,  // true if we have fixed container. If we do so we will need to se fixed_num_buckets and fixed_num_elements.
     * }
     *
     * static inline const key_type& key_from_value(const value_type& value);
     */
    template <class Traits>
    class rbtree
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        typedef rbtree<Traits>                  this_type;
    protected:
        typedef Internal::rbtree_node_base*         base_node_ptr_type;
        typedef const Internal::rbtree_node_base*   const_base_node_ptr_type;
        //typedef rbtree_color_type             color_type;
    public:
        typedef Traits                          traits_type;

        typedef typename Traits::key_type       key_type;
        typedef typename Traits::key_equal         key_equal;
        typedef typename Traits::allocator_type allocator_type;
        typedef typename allocator_type::size_type size_type;
        typedef typename allocator_type::difference_type difference_type;

        typedef typename Traits::value_type     value_type;
        typedef value_type*                     pointer;
        typedef const value_type*               const_pointer;
        typedef value_type&                     reference;
        typedef const value_type&               const_reference;

        typedef AZStd::bidirectional_iterator_tag iterator_category;

        typedef Internal::rbtree_node<value_type>   node_type;
        typedef node_type*                          node_ptr_type;
        typedef const node_type*                    const_node_ptr_type;

        typedef rbtree_const_iterator<typename Traits::value_type>  const_iterator_impl;
        typedef rbtree_iterator<typename Traits::value_type>        iterator_impl;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_bidirectional_iterator<iterator_impl, this_type>          iterator;
        typedef Debug::checked_bidirectional_iterator<const_iterator_impl, this_type>    const_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
#endif

        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;

        typedef rbtree_node_destructor<allocator_type, node_type> node_deleter;

    public:
        AZ_FORCE_INLINE rbtree()
            : m_numElements(0)
            , m_keyEq(key_equal())
            , m_allocator(allocator_type())
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(0);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
        }
        explicit rbtree(const key_equal& keyEq)
            : m_numElements(0)
            , m_keyEq(keyEq)
            , m_allocator(allocator_type())
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(nullptr);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
        }
        explicit rbtree(const allocator_type& allocator)
            : m_numElements{}
            , m_keyEq{}
            , m_allocator{ allocator }
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(nullptr);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
        }
        AZ_FORCE_INLINE rbtree(const key_equal& keyEq, const allocator_type& allocator)
            : m_numElements(0)
            , m_keyEq(keyEq)
            , m_allocator(allocator)
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(0);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
        }
        inline rbtree(const this_type& rhs)
            : m_numElements(rhs.m_numElements)
            , m_keyEq(rhs.m_keyEq)
            , m_allocator(rhs.m_allocator)
        {
            m_head.set_color(AZSTD_RBTREE_RED);
            if (rhs.m_head.get_parent() != 0)
            {
                base_node_ptr_type parent = copy(rhs.m_head.get_parent(), &m_head);
                m_head.set_parent(parent);

                m_head.m_left = parent->minimum();
                m_head.m_right = parent->maximum();
            }
            else
            {
                m_head.set_parent(0);
                m_head.m_left = &m_head;
                m_head.m_right = &m_head;
            }
        }
        inline rbtree(const this_type& rhs, const allocator_type& allocator)
            : m_numElements(rhs.m_numElements)
            , m_keyEq(rhs.m_keyEq)
            , m_allocator(allocator)
        {
            m_head.set_color(AZSTD_RBTREE_RED);
            if (rhs.m_head.get_parent() != 0)
            {
                base_node_ptr_type parent = copy(rhs.m_head.get_parent(), &m_head);
                m_head.set_parent(parent);

                m_head.m_left = parent->minimum();
                m_head.m_right = parent->maximum();
            }
            else
            {
                m_head.set_parent(0);
                m_head.m_left = &m_head;
                m_head.m_right = &m_head;
            }
        }
        AZ_FORCE_INLINE ~rbtree() { clear(); }
        this_type& operator=(const this_type& rhs)
        {
            if (this != &rhs)
            {
                clear();
                m_numElements = 0;
                m_keyEq = rhs.m_keyEq;
                if (rhs.m_head.get_parent() == 0)
                {
                    m_head.set_parent(0);
                    m_head.m_left = &m_head;
                    m_head.m_right = &m_head;
                }
                else
                {
                    m_head.set_parent(copy(rhs.m_head.get_parent(), &m_head));
                    m_head.m_left = m_head.get_parent()->minimum();
                    m_head.m_right = m_head.get_parent()->maximum();
                    m_numElements = rhs.m_numElements;
                }
            }
            return *this;
        }

        // accessors:
        AZ_FORCE_INLINE key_equal  key_comp() const                { return m_keyEq; }

        AZ_FORCE_INLINE iterator begin()                        { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_head.m_left)); }
        AZ_FORCE_INLINE const_iterator begin() const            { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, m_head.m_left)); }
        AZ_FORCE_INLINE iterator end()                          { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, &m_head)); }
        AZ_FORCE_INLINE const_iterator end() const              { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<base_node_ptr_type>(&m_head))); }

        AZ_FORCE_INLINE reverse_iterator rbegin()               { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const   { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator rend()                 { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const     { return const_reverse_iterator(begin());   }

        AZ_FORCE_INLINE bool empty() const                      { return m_numElements == 0; }
        AZ_FORCE_INLINE size_type size() const                  { return m_numElements; }
        AZ_FORCE_INLINE size_type max_size() const              { return AZStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(node_type); }

        rbtree(this_type&& rhs)
            : m_numElements(0) // it will be set during swap
            , m_keyEq(AZStd::move(rhs.m_keyEq))
            , m_allocator(AZStd::move(rhs.m_allocator))
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(0);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
            swap(rhs);
        }

        rbtree(this_type&& rhs, const allocator_type& allocator)
            : m_numElements(0)
            , m_keyEq(AZStd::move(rhs.m_keyEq))
            , m_allocator(allocator)
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(0);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
            swap(rhs);
        }

        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }

        AZStd::pair<iterator, bool> insert_unique(value_type&& value)
        {
            bool isInserted = false;
            iterator insertPos = lower_bound(Traits::key_from_value(value));
            base_node_ptr_type insertNode = insertPos.m_node;
            if (insertNode != &m_head) // !end()
            {
                if (m_keyEq(Traits::key_from_value(value), Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value)))
                {
                    // insert_unique should not fail (as we already searched).
                    insertPos = insert_unique_node(insertPos, create_node(AZStd::forward<value_type>(value)));
                    isInserted = true;
                }
            }
            else
            {
                insertPos = insert_unique_node(insertPos, create_node(AZStd::forward<value_type>(value)));
                isInserted = true;
            }
            return AZStd::pair<iterator, bool>(insertPos, isInserted);
        }

        iterator insert_unique(const_iterator insertPos, value_type&& value)
        {
            // this is not efficient, pass the value for clone on success or just ignore the
            // insertPos and do a search (which should be fine and insertPos is a hit, but in practice
            // we expect most people will call with insertPos if the node can't be inserted)
            node_ptr_type newNode = static_cast<node_ptr_type>(create_node(AZStd::move(value)));
            iterator result = insert_unique_node(insertPos, newNode);
            if (result == insertPos)
            {
                pointer toDestroy = &newNode->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(newNode, allocator::allow_memory_leaks());
            }
            return result;
        }

        iterator insert_equal(value_type&& value)
        {
            return insert_equal_node(create_node(AZStd::forward<value_type>(value)));
        }

        template<class ... InputArguments>
        AZStd::pair<iterator, bool> emplace_unique(InputArguments&& ... arguments)
        {
            // we need to construct the node just to get the key/value
            node_ptr_type newNode = static_cast<node_ptr_type>(create_node(AZStd::forward<InputArguments>(arguments) ...));
            AZStd::pair<iterator, bool> result = insert_unique_node(newNode);
            if (!result.second)
            {
                pointer toDestroy = &newNode->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(newNode, allocator::allow_memory_leaks());
            }
            return result;
        }

        template<class ... InputArguments>
        iterator emplace_unique(const_iterator insertPos, InputArguments&& ... arguments)
        {
            // we need to construct the node just to get the key/value
            node_ptr_type newNode = static_cast<node_ptr_type>(create_node(AZStd::forward<InputArguments>(arguments) ...));
            iterator result = insert_unique_node(insertPos, newNode);
            if (result == insertPos)
            {
                pointer toDestroy = &newNode->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(newNode, allocator::allow_memory_leaks());
            }
            return result;
        }

        template<class ... InputArguments>
        iterator emplace_equal(InputArguments&& ... arguments)
        {
            return insert_equal_node(create_node(AZStd::forward<InputArguments>(arguments) ...));
        }

        template<class ... InputArguments>
        iterator emplace_equal(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return insert_equal_node(insertPos, create_node(AZStd::forward<InputArguments>(arguments) ...));
        }

        inline void swap(this_type& rhs)
        {
            if (this == &rhs)
            {
                return;
            }

            if (m_allocator == rhs.m_allocator)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
                if (rhs.m_numElements == 0)
                {
                    if (m_numElements == 0)
                    {
                        return;
                    }

                    AZStd::swap(m_head, rhs.m_head);
                    rhs.rebind(&m_head);
                    // empty init
                    m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
                    m_head.set_parent(0);
                    m_head.m_left = &m_head;
                    m_head.m_right = &m_head;
                }
                else if (m_numElements == 0)
                {
                    rhs.swap(*this);
                    return;
                }
                else
                {
                    AZStd::swap(m_head, rhs.m_head);
                    rebind(&rhs.m_head);
                    rhs.rebind(&m_head);
                }
                AZStd::swap(m_numElements, rhs.m_numElements);
                AZStd::swap(m_keyEq, rhs.m_keyEq);
            }
            else
            {
                // Different allocators, move elements
                rbtree temp(m_keyEq ,m_allocator);
                // Move this rbtree elements to a temp rbtree
                base_node_ptr_type parent = m_head.get_parent();
                if (parent)
                {
                    temp.m_head.set_parent(temp.move(parent, &temp.m_head));
                    temp.m_head.m_left = temp.m_head.get_parent()->minimum();
                    temp.m_head.m_right = temp.m_head.get_parent()->maximum();
                    temp.m_numElements = m_numElements;
                }

                // Move rhs rbtree elements to this rbtree
                clear();
                base_node_ptr_type rhsParent = rhs.m_head.get_parent();
                if (rhsParent)
                {
                    m_head.set_parent(move(rhsParent, &m_head));
                    m_head.m_left = m_head.get_parent()->minimum();
                    m_head.m_right = m_head.get_parent()->maximum();
                    m_numElements = rhs.m_numElements;
                }
                // Move temp rbtree elements to rhs rbtree
                rhs.clear();
                base_node_ptr_type tempParent = temp.m_head.get_parent();
                if (tempParent)
                {
                    rhs.m_head.set_parent(rhs.move(tempParent, &rhs.m_head));
                    rhs.m_head.m_left = rhs.m_head.get_parent()->minimum();
                    rhs.m_head.m_right = rhs.m_head.get_parent()->maximum();
                    rhs.m_numElements = temp.m_numElements;
                }
            }
        }

        // insert/erase
        AZStd::pair<iterator, bool> insert_unique(const value_type& value)
        {
            bool isInserted = false;
            iterator insertPos = lower_bound(Traits::key_from_value(value));
            base_node_ptr_type insertNode = insertPos.m_node;
            if (insertNode != &m_head) // !end()
            {
                if (m_keyEq(Traits::key_from_value(value), Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value)))
                {
                    // insert_unique should not fail (as we already searched).
                    insertPos = insert_unique_node(insertPos, create_node(value));
                    isInserted = true;
                }
            }
            else
            {
                insertPos = insert_unique_node(insertPos, create_node(value));
                isInserted = true;
            }
            return AZStd::pair<iterator, bool>(insertPos, isInserted);
        }

        iterator insert_equal(const value_type& value)
        {
            return insert_equal_node(create_node(value));
        }

        iterator insert_unique(const_iterator insertPos, const value_type& value)
        {
            // this is not efficient, pass the value for clone on sucess or just ignore the
            // insertPos and do a search (which should be fine and insertPos is a hit, but in practice
            // we except most people will call with insertPos if the node can't be inserted)
            node_ptr_type newNode = static_cast<node_ptr_type>(create_node(value));
            iterator result = insert_unique_node(insertPos, newNode);
            if (result == insertPos)
            {
                pointer toDestroy = &newNode->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(newNode, allocator::allow_memory_leaks());
            }
            return result;
        }
        iterator insert_equal(const iterator insertPos, const value_type& value)
        {
            return insert_equal_node(insertPos, create_node(value));
        }

        template<class Iterator>
        AZ_FORCE_INLINE void insert_equal(Iterator first, Iterator last)
        {
            for (; first != last; ++first)
            {
                insert_equal(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE void insert_unique(Iterator first, Iterator last)
        {
            for (; first != last; ++first)
            {
                insert_unique(*first);
            }
        }

        template <typename ComparableToKey, typename... Args>
        AZStd::pair<iterator, bool> try_emplace_unique(ComparableToKey&& key, Args&&... arguments);

        template <typename ComparableToKey, typename... Args>
        iterator try_emplace_unique(const_iterator hint, ComparableToKey&& key, Args&&... arguments);

        template <typename ComparableToKey, typename MappedType>
        AZStd::pair<iterator, bool> insert_or_assign_unique(ComparableToKey&& key, MappedType&& value);

        template <typename ComparableToKey, typename MappedType>
        iterator insert_or_assign_unique(const_iterator hint, ComparableToKey&& key, MappedType&& value);

        //! Returns an insert_return_type with the members initialized as follows: if nodeHandle is empty, inserted is false, position is end(), and node is empty.
        //! Otherwise if the insertion took place, inserted is true, position points to the inserted element, and node is empty.
        //! If the insertion failed, inserted is false, node has the previous value of nodeHandle, and position points to an element with a key equivalent to nodeHandle.key().
        template <class InsertReturnType, class NodeHandle>
        InsertReturnType node_handle_insert_unique(NodeHandle&& nodeHandle);
        template <class NodeHandle>
        auto node_handle_insert_unique(const iterator hint, NodeHandle&& nodeHandle) -> iterator;

        //! Returns an iterator pointing to the inserted element.
        //! If the nodeHandle is empty the end() iterator is returned
        template <class NodeHandle>
        auto node_handle_insert_equal(NodeHandle&& nodeHandle) -> iterator;
        template <class NodeHandle>
        auto node_handle_insert_equal(const iterator hint, NodeHandle&& nodeHandle) -> iterator;

        //! Searches for an element which matches the value of key and extracts it from the hash_table
        //! @return A NodeHandle which can be used to insert the an element between unique and non-unique containers of the same type
        //! i.e a NodeHandle from an unordered_map can be used to insert a node into an unordered_multimap, but not a std::map
        template <class NodeHandle>
        NodeHandle node_handle_extract(const key_type& key);
        //! Finds an element within the hash_table that is represented by the supplied iterator and extracts it
        //! @return A NodeHandle which can be used to insert the an element between unique and non-unique containers of the same type
        template <class NodeHandle>
        NodeHandle node_handle_extract(const_iterator it);

        inline iterator erase(const_iterator erasePos)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type eraseNode = erasePos.get_iterator().m_node;
            orphan_node(eraseNode);
#else
            node_ptr_type eraseNode = erasePos.m_node;
#endif
            iterator next(AZSTD_CHECKED_ITERATOR(iterator_impl, eraseNode));
            ++next;
            node_ptr_type nodeToErase =  static_cast<node_ptr_type>(rebalance_for_erase(eraseNode, m_head.m_left, m_head.m_right));

            pointer toDestroy = &nodeToErase->m_value;
            Internal::destroy<pointer>::single(toDestroy);
            deallocate_node(nodeToErase, allocator::allow_memory_leaks());
            --m_numElements;
            return next;
        }

        inline size_type erase(const key_type& key)
        {
            AZStd::pair<iterator, iterator> p = equal_range(key);
            size_type n = AZStd::distance(p.first, p.second);
            erase(p.first, p.second);
            return n;
        }

        inline bool erase_unique(const key_type& key)
        {
            iterator iter = find(key);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type eraseNode = iter.get_iterator().m_node;
#else
            base_node_ptr_type eraseNode = iter.m_node;
#endif
            if (eraseNode != &m_head)
            {
                erase(iter);
                return true;
            }
            return false;
        }

        inline iterator erase(const_iterator first, const_iterator last)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type firstNode = first.get_iterator().m_node;
            base_node_ptr_type lastNode = last.get_iterator().m_node;
#else
            base_node_ptr_type firstNode = first.m_node;
            base_node_ptr_type lastNode = last.m_node;
#endif
            if (firstNode == m_head.m_left && // begin()
                lastNode == &m_head)    // end()
            {
                clear();
            }
            else
            {
                while (first != last)
                {
                    erase(first++);
                }
            }

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, lastNode));
        }

        AZ_FORCE_INLINE void erase(const key_type* first, const key_type* last)
        {
            while (first != last)
            {
                erase(*first++);
            }
        }

        AZ_FORCE_INLINE void clear()
        {
            if (m_numElements != 0)
            {
                erase(m_head.get_parent());
                m_head.set_parent(0);
                m_head.m_left = &m_head;
                m_head.m_right = &m_head;
                m_numElements = 0;
            }
        }

        template<class ComparableToKey>
        auto find(const ComparableToKey& key)
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            base_node_ptr_type y = &m_head;
            base_node_ptr_type x = m_head.get_parent();

            while (x != 0)
            {
                if (!m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(x)->m_value), key))
                {
                    y = x, x = x->m_left;
                }
                else
                {
                    x = x->m_right;
                }
            }

            if (y != &m_head)
            {
                if (m_keyEq(key, Traits::key_from_value(static_cast<node_ptr_type>(y)->m_value)))
                {
                    y = &m_head;
                }
            }
            return iterator(y);
        }

        template<class ComparableToKey>
        auto find(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return const_iterator(const_cast<rbtree*>(this)->find(key));
        }

        template<class ComparableToKey>
        auto contains(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, bool>
        {
            return find(key) != end();
        }

        template<class ComparableToKey>
        auto lower_bound(const ComparableToKey& key)
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            base_node_ptr_type y = &m_head;
            base_node_ptr_type x = m_head.get_parent();

            while (x != 0)
            {
                if (!m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(x)->m_value), key))
                {
                    y = x, x = x->m_left;
                }
                else
                {
                    x = x->m_right;
                }
            }

            return iterator(y);
        }

        template<class ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return const_iterator(const_cast<rbtree*>(this)->lower_bound(key));
        }

        template<class ComparableToKey>
        auto upper_bound(const ComparableToKey& key)
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, iterator>
        {
            base_node_ptr_type y = &m_head;
            base_node_ptr_type x = m_head.get_parent();

            while (x != 0)
            {
                if (m_keyEq(key, Traits::key_from_value(static_cast<node_ptr_type>(x)->m_value)))
                {
                    y = x, x = x->m_left;
                }
                else
                {
                    x = x->m_right;
                }
            }

            return iterator(y);
        }

        template<class ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, const_iterator>
        {
            return const_iterator(const_cast<rbtree*>(this)->upper_bound(key));
        }

        template<class ComparableToKey>
        auto count(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, size_type>
        {
            AZStd::pair<const_iterator, const_iterator> p = equal_range(key);
            return AZStd::distance(p.first, p.second);
        }

        template<class ComparableToKey>
        auto equal_range(const ComparableToKey& key)
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<iterator, iterator>>
        {
            return { lower_bound(key), upper_bound(key) };
        }
        template<class ComparableToKey>
        auto equal_range(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<const_iterator, const_iterator>>
        {
            return { lower_bound(key), upper_bound(key) };
        }

        template<class ComparableToKey>
        auto equal_range_unique(const ComparableToKey& key)
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<iterator, iterator>>
        {
            AZStd::pair<iterator, iterator> p;
            p.second = lower_bound(key);
            base_node_ptr_type lowerNode = p.second.m_node;
            if (lowerNode != &m_head && !m_keyEq(key, Traits::key_from_value(static_cast<node_ptr_type>(lowerNode)->m_value)))
            {
                p.first = p.second++;
            }
            else
            {
                p.first = p.second;
            }
            return p;
        }
        template<class ComparableToKey>
        auto equal_range_unique(const ComparableToKey& key) const
            -> enable_if_t<Internal::is_transparent<key_equal, ComparableToKey>::value || AZStd::is_convertible_v<ComparableToKey, key_type>, AZStd::pair<const_iterator, const_iterator>>
        {
            AZStd::pair<iterator, iterator> non_const_range(const_cast<rbtree*>(this)->equal_range_unique(key));
            return { const_iterator(non_const_range.first), const_iterator(non_const_range.second) };
        }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        // The only difference from the standard is that we return the allocator instance, not a copy.
        inline allocator_type&          get_allocator()         { return m_allocator; }
        inline const allocator_type&    get_allocator() const   { return m_allocator; }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        inline void                     set_allocator(const allocator_type& allocator)
        {
            if (m_allocator != allocator)
            {
                if (m_numElements > 0)
                {
                    // create temp list with all elements relocated.
                    this_type templist(begin(), end(), allocator);
                    clear();
                    m_allocator = allocator;
                    swap(templist);
                }
                else
                {
                    m_allocator = allocator;
                }
            }
        }

        // Validate container status.
        inline bool     validate() const
        {
            // \noto \todo traverse the list and make sure it's properly linked.
            return true;
        }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            node_ptr_type iterNode = iter.m_iter.m_node;
#else
            node_ptr_type iterNode = iter.m_node;
#endif
            if (iterNode == (node_ptr_type) & m_head)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        inline int      validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            node_ptr_type iterNode = iter.m_iter.m_node;
#else
            node_ptr_type iterNode = iter.m_node;
#endif
            if (iterNode == (node_ptr_type) & m_head)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        inline void                     leak_and_reset()
        {
            m_head.set_color(AZSTD_RBTREE_RED); // used to distinguish header from root, in iterator.operator++
            m_head.set_parent(0);
            m_head.m_left = &m_head;
            m_head.m_right = &m_head;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

        /// @}

    private:
        inline base_node_ptr_type create_node(const value_type& value)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(m_allocator.allocate(sizeof(node_type), alignment_of<node_type>::value));
            AZSTD_CONTAINER_ASSERT(newNode != NULL, "AZStd::rb_tree::create_node - failed to allocate node!");

            // copy construct
            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr, value);

            newNode->m_left = 0;
            newNode->m_right = 0;
            return newNode;
        }

        template<class ... InputArguments>
        inline base_node_ptr_type create_node(InputArguments&& ... arguments)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(m_allocator.allocate(sizeof(node_type), alignment_of<node_type>::value));
            AZSTD_CONTAINER_ASSERT(newNode, "AZStd::rb_tree::create_node - failed to allocate node!");

            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr, AZStd::forward<InputArguments>(arguments) ...);

            newNode->m_left = 0;
            newNode->m_right = 0;
            return newNode;
        }

        AZ_FORCE_INLINE base_node_ptr_type clone_node(base_node_ptr_type node)
        {
            base_node_ptr_type tmp = create_node(static_cast<node_ptr_type>(node)->m_value);
            tmp->set_color(node->get_color());
            return tmp;
        }

        base_node_ptr_type move_node(base_node_ptr_type node)
        {
            base_node_ptr_type tmp = create_node(AZStd::move(static_cast<node_ptr_type>(node)->m_value));
            tmp->set_color(node->get_color());
            return tmp;
        }

        iterator insert_node(base_node_ptr_type parent, base_node_ptr_type newNode, base_node_ptr_type on_left = 0, base_node_ptr_type on_right = 0)
        {
            if (parent == &m_head)
            {
                parent->m_left = newNode;   // also makes m_leftmost() = __new_node
                m_head.set_parent(newNode);
                m_head.m_right = newNode;
            }
            else if (on_right == 0 &&      // If __on_right != 0, the remainder fails to false
                     (on_left != 0 || // If __on_left != 0, the remainder succeeds to true
                      m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(parent)->m_value))))
            {
                parent->m_left = newNode;

                if (parent == m_head.m_left)
                {
                    m_head.m_left = newNode;
                }
            }
            else
            {
                parent->m_right = newNode;
                if (parent == m_head.m_right)
                {
                    m_head.m_right = newNode;  // maintain rightmost() pointing to max node
                }
            }

            newNode->set_parent(parent);
            rebalance(newNode);
            ++m_numElements;
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, newNode));
        }

        AZStd::pair<iterator, bool> insert_unique_node(base_node_ptr_type newNode)
        {
            base_node_ptr_type y = &m_head;
            base_node_ptr_type x = m_head.get_parent();
            bool comp = true;
            while (x != 0)
            {
                y = x;
                comp = m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(x)->m_value));
                x = comp ? x->m_left : x->m_right;
            }

            iterator j(AZSTD_CHECKED_ITERATOR(iterator_impl, y));

            if (comp)
            {
                if (j == begin())
                {
                    return AZStd::pair<iterator, bool>(insert_node(y, newNode, y), true);
                }
                else
                {
                    --j;
                }
            }

            if (m_keyEq(Traits::key_from_value(*j), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value)))
            {
                return AZStd::pair<iterator, bool>(insert_node(y, newNode, x), true);
            }
            return AZStd::pair<iterator, bool>(j, false);
        }

        iterator insert_equal_node(base_node_ptr_type newNode)
        {
            base_node_ptr_type y = &m_head;
            base_node_ptr_type x = m_head.get_parent();
            while (x != 0)
            {
                y = x;
                if (m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(x)->m_value)))
                {
                    x = x->m_left;
                }
                else
                {
                    x = x->m_right;
                }
            }
            return insert_node(y, newNode, x);
        }


        iterator insert_unique_node(const_iterator insertPos, base_node_ptr_type newNode)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insertNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type insertNode = insertPos.m_node;
#endif
            if (insertNode == this->m_head.m_left)
            { // begin()
              // if the container is empty, fall back on insert_unique.
                if (m_numElements == 0)
                {
                    return insert_unique_node(newNode).first;
                }

                if (m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value)))
                {
                    return insert_node(insertNode, newNode, insertNode);
                }
                // first argument just needs to be non-null
                else
                {
                    bool comp_pos_v = m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value));
                    if (comp_pos_v == false)  // compare > and compare < both false so compare equal
                    {
                        return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                    }
                    //Below __comp_pos_v == true

                    // Standard-conformance - does the insertion point fall immediately AFTER
                    // the hint?
                    iterator after(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                    ++after;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                    base_node_ptr_type afterNode = after.get_iterator().m_node;
#else
                    base_node_ptr_type afterNode = after.m_node;
#endif
                    // Check for only one member -- in that case, __position points to itself,
                    // and attempting to increment will cause an infinite loop.
                    if (afterNode == &m_head)
                    {
                        // Check guarantees exactly one member, so comparison was already
                        // performed and we know the result; skip repeating it in insert
                        // by specifying a non-zero fourth argument.
                        return insert_node(insertNode, newNode, 0, insertNode);
                    }

                    // All other cases:

                    // Optimization to catch insert-equivalent -- save comparison results,
                    // and we get this for free.
                    if (m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(afterNode)->m_value)))
                    {
                        if (insertNode->m_right == 0)
                        {
                            return insert_node(insertNode, newNode, 0, insertNode);
                        }
                        else
                        {
                            return insert_node(afterNode, newNode, afterNode);
                        }
                    }
                    else
                    {
                        return insert_unique_node(newNode).first;
                    }
                }
            }
            else if (insertNode == &m_head)
            { // end()
                if (m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(m_head.m_right)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value)))
                {
                    // pass along to insert that it can skip comparing
                    // v, Key ; since compare Key, v was true, compare v, Key must be false.
                    return insert_node(m_head.m_right, newNode, 0, m_head.m_right); // Last argument only needs to be non-null
                }
                else
                {
                    return insert_unique_node(newNode).first;
                }
            }
            else
            {
                iterator before(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                --before;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                base_node_ptr_type beforeNode = before.get_iterator().m_node;
#else
                base_node_ptr_type beforeNode = before.m_node;
#endif
                bool comp_v_pos = m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value));

                if (comp_v_pos && m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(beforeNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value)))
                {
                    if (beforeNode->m_right == 0)
                    {
                        return insert_node(beforeNode, newNode, 0, beforeNode); // Last argument only needs to be non-null
                    }
                    else
                    {
                        return insert_node(insertNode, newNode, insertNode);
                    }
                    // first argument just needs to be non-null
                }
                else
                {
                    // Does the insertion point fall immediately AFTER the hint?
                    iterator after(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                    ++after;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                    base_node_ptr_type afterNode = after.get_iterator().m_node;
#else
                    base_node_ptr_type afterNode = after.m_node;
#endif
                    // Optimization to catch equivalent cases and avoid unnecessary comparisons
                    bool comp_pos_v = !comp_v_pos;  // Stored this result earlier
                    // If the earlier comparison was true, this comparison doesn't need to be
                    // performed because it must be false.  However, if the earlier comparison
                    // was false, we need to perform this one because in the equal case, both will
                    // be false.
                    if (!comp_v_pos)
                    {
                        comp_pos_v = m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value));
                    }

                    if ((!comp_v_pos) // comp_v_pos true implies comp_v_pos false
                        && comp_pos_v
                        && (afterNode == &m_head ||
                            m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(afterNode)->m_value))))
                    {
                        if (insertNode->m_right == 0)
                        {
                            return insert_node(insertNode, newNode, 0, insertNode);
                        }
                        else
                        {
                            return insert_node(afterNode, newNode, afterNode);
                        }
                    }
                    else
                    {
                        // Test for equivalent case
                        if (comp_v_pos == comp_pos_v)
                        {
                            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                        }
                        else
                        {
                            return insert_unique_node(newNode).first;
                        }
                    }
                }
            }
        }
        iterator insert_equal_node(const_iterator insertPos, base_node_ptr_type newNode)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insertNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type insertNode = insertPos.m_node;
#endif
            if (insertNode == m_head.m_left)
            { // begin()
              // Check for zero members
                if (m_numElements <= 0)
                {
                    return insert_equal_node(newNode);
                }

                if (!m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value)))
                {
                    return insert_node(insertNode, newNode, insertNode);
                }
                else
                {
                    // Check for only one member
                    if (insertNode->m_left == insertNode)
                    {
                        // Unlike insert_unique, can't avoid doing a comparison here.
                        return insert_node(insertNode, newNode);
                    }

                    // All other cases:
                    // Standard-conformance - does the insertion point fall immediately AFTER
                    // the hint?
                    iterator after(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                    ++after;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                    base_node_ptr_type afterNode = after.get_iterator().m_node;
#else
                    base_node_ptr_type afterNode = after.m_node;
#endif
                    // Already know that compare(pos, v) must be true!
                    // Therefore, we want to know if compare(after, v) is false.
                    // (i.e., we now pos < v, now we want to know if v <= after)
                    // If not, invalid hint.
                    if (afterNode == &m_head || !m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(afterNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value)))
                    {
                        if (insertNode->m_right == 0)
                        {
                            return insert_node(insertNode, newNode, 0, insertNode);
                        }
                        else
                        {
                            return insert_node(afterNode, newNode, afterNode);
                        }
                    }
                    else
                    { // Invalid hint
                        return insert_equal_node(newNode);
                    }
                }
            }
            else if (insertNode == &m_head)   // end()
            {
                if (!m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(m_head.m_right)->m_value)))
                {
                    return insert_node(m_head.m_right, newNode, 0, insertNode); // Last argument only needs to be non-null
                }
                else
                {
                    return insert_equal_node(newNode);
                }
            }
            else
            {
                iterator before(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                --before;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                base_node_ptr_type beforeNode = before.get_iterator().m_node;
#else
                base_node_ptr_type beforeNode = before.m_node;
#endif
                // store the result of the comparison between pos and v so
                // that we don't have to do it again later.  Note that this reverses the shortcut
                // on the if, possibly harming efficiency in comparisons; I think the harm will
                // be negligible, and to do what I want to do (save the result of a comparison so
                // that it can be re-used) there is no alternative.  Test here is for before <= v <= pos.
                bool comp_pos_v = m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(insertNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value));
                if (!comp_pos_v && !m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(beforeNode)->m_value)))
                {
                    if (beforeNode->m_right == 0)
                    {
                        return insert_node(beforeNode, newNode, 0, beforeNode); // Last argument only needs to be non-null
                    }
                    else
                    {
                        return insert_node(insertNode, newNode, insertNode);
                    }
                }
                else
                {
                    // Does the insertion point fall immediately AFTER the hint?
                    // Test for pos < v <= after
                    iterator after(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.m_node));
                    ++after;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                    base_node_ptr_type afterNode = after.get_iterator().m_node;
#else
                    base_node_ptr_type afterNode = after.m_node;
#endif
                    if (comp_pos_v && (afterNode == &m_head || !m_keyEq(Traits::key_from_value(static_cast<node_ptr_type>(afterNode)->m_value), Traits::key_from_value(static_cast<node_ptr_type>(newNode)->m_value))))
                    {
                        if (insertNode->m_right == 0)
                        {
                            return insert_node(insertNode, newNode, 0, insertNode);
                        }
                        else
                        {
                            return insert_node(afterNode, newNode, afterNode);
                        }
                    }
                    else
                    { // Invalid hint
                        return insert_equal_node(newNode);
                    }
                }
            }
        }

        base_node_ptr_type copy(base_node_ptr_type node, base_node_ptr_type parent)
        {
            // structural copy.  node and parent must be non-null.
            base_node_ptr_type top = clone_node(node);
            top->set_parent(parent);

            if (node->m_right)
            {
                top->m_right = copy(node->m_right, top);
            }
            parent = top;
            node = node->m_left;

            while (node != 0)
            {
                base_node_ptr_type y = clone_node(node);
                parent->m_left = y;
                y->set_parent(parent);
                if (node->m_right)
                {
                    y->m_right = copy(node->m_right, y);
                }
                parent = y;
                node = node->m_left;
            }
            return top;
        }

        base_node_ptr_type move(base_node_ptr_type node, base_node_ptr_type parent)
        {
            // structural move.  node and parent must be non-null.
            base_node_ptr_type top = move_node(node);
            top->set_parent(parent);

            if (node->m_right)
            {
                top->m_right = move(node->m_right, top);
            }
            parent = top;
            node = node->m_left;

            while (node)
            {
                base_node_ptr_type movedNode = move_node(node);
                parent->m_left = movedNode;
                movedNode->set_parent(parent);
                if (node->m_right)
                {
                    movedNode->m_right = move(node->m_right, movedNode);
                }
                parent = movedNode;
                node = node->m_left;
            }
            return top;
        }

        void erase(base_node_ptr_type node)
        {
            // erase without rebalancing
            while (node != 0)
            {
                erase(node->m_right);
                base_node_ptr_type y = node->m_left;
                node_ptr_type nodeToErase = static_cast<node_ptr_type>(node);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_node(nodeToErase);
#endif
                pointer toDestroy = &nodeToErase->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(nodeToErase, allocator::allow_memory_leaks());
                node = y;
            }
        }

        void rebind(base_node_ptr_type node)
        {
            base_node_ptr_type parent = m_head.get_parent();
            if (parent != 0)
            {
                parent->set_parent(&m_head);
            }
            if (m_head.m_right == node)
            {
                m_head.m_right = &m_head;
            }
            if (m_head.m_left == node)
            {
                m_head.m_left = &m_head;
            }
        }

        AZ_FORCE_INLINE void    deallocate_node(node_ptr_type node, const true_type& /* allocator::allow_memory_leaks */)
        {
            (void)node;
        }

        AZ_FORCE_INLINE void    deallocate_node(node_ptr_type node, const false_type& /* !allocator::allow_memory_leaks */)
        {
            m_allocator.deallocate(node, sizeof(node_type), alignment_of<node_type>::value);
        }

        inline void rotate_left(base_node_ptr_type x)
        {
            base_node_ptr_type y = x->m_right;
            x->m_right = y->m_left;
            if (y->m_left != 0)
            {
                y->m_left->set_parent(x);
            }

            base_node_ptr_type xParent = x->get_parent();
            y->set_parent(xParent);

            if (x == m_head.get_parent())
            {
                m_head.set_parent(y);
            }

            else if (x == xParent->m_left)
            {
                xParent->m_left = y;
            }
            else
            {
                xParent->m_right = y;
            }
            y->m_left = x;
            x->set_parent(y);
        }

        inline void rotate_right(base_node_ptr_type x)
        {
            base_node_ptr_type y = x->m_left;
            x->m_left = y->m_right;
            if (y->m_right != 0)
            {
                y->m_right->set_parent(x);
            }
            base_node_ptr_type xParent = x->get_parent();
            y->set_parent(xParent);

            if (x == m_head.get_parent())
            {
                m_head.set_parent(y);
            }
            else if (x == xParent->m_right)
            {
                xParent->m_right = y;
            }
            else
            {
                xParent->m_left = y;
            }
            y->m_right = x;
            x->set_parent(y);
        }

        void rebalance(base_node_ptr_type x)
        {
            x->set_color(AZSTD_RBTREE_RED);
            base_node_ptr_type xParent = x->get_parent();
            while (x != m_head.get_parent() && xParent->get_color() == AZSTD_RBTREE_RED)
            {
                base_node_ptr_type xParentParent = xParent->get_parent();
                if (xParent == xParentParent->m_left)
                {
                    base_node_ptr_type y = xParentParent->m_right;
                    if (y && y->get_color() == AZSTD_RBTREE_RED)
                    {
                        xParent->set_color(AZSTD_RBTREE_BLACK);
                        y->set_color(AZSTD_RBTREE_BLACK);
                        xParentParent->set_color(AZSTD_RBTREE_RED);
                        x = xParentParent;
                        xParent = x->get_parent();
                    }
                    else
                    {
                        if (x == xParent->m_right)
                        {
                            x = xParent;
                            rotate_left(x);
                            xParent = x->get_parent();
                            xParentParent = xParent->get_parent();
                        }
                        xParent->set_color(AZSTD_RBTREE_BLACK);
                        xParentParent->set_color(AZSTD_RBTREE_RED);
                        rotate_right(xParentParent);
                        xParent = x->get_parent();
                    }
                }
                else
                {
                    base_node_ptr_type y = xParentParent->m_left;
                    if (y && y->get_color() == AZSTD_RBTREE_RED)
                    {
                        xParent->set_color(AZSTD_RBTREE_BLACK);
                        y->set_color(AZSTD_RBTREE_BLACK);
                        xParentParent->set_color(AZSTD_RBTREE_RED);
                        x = xParentParent;
                        xParent = x->get_parent();
                    }
                    else
                    {
                        if (x == xParent->m_left)
                        {
                            x = xParent;
                            rotate_right(x);
                            xParent = x->get_parent();
                            xParentParent = xParent->get_parent();
                        }
                        xParent->set_color(AZSTD_RBTREE_BLACK);
                        xParentParent->set_color(AZSTD_RBTREE_RED);
                        rotate_left(xParentParent);
                        xParent = x->get_parent();
                    }
                }
            }
            m_head.get_parent()->set_color(AZSTD_RBTREE_BLACK);
        }

        base_node_ptr_type rebalance_for_erase(base_node_ptr_type z, base_node_ptr_type& leftmost, base_node_ptr_type& rightmost)
        {
            base_node_ptr_type y = z;
            base_node_ptr_type x;
            base_node_ptr_type xParent;

            if (y->m_left == 0)     // z has at most one non-null child. y == z.
            {
                x = y->m_right;     // x might be null.
            }
            else
            {
                if (y->m_right == 0)  // z has exactly one non-null child. y == z.
                {
                    x = y->m_left;    // x is not null.
                }
                else                     // z has two non-null children.  Set y to
                {
                    y = y->m_right->minimum();   //   z's successor.  x might be null.
                    x = y->m_right;
                }
            }

            if (y != z)
            {   // relink y in place of z.  y is z's successor
                z->m_left->set_parent(y);
                y->m_left = z->m_left;
                if (y != z->m_right)
                {
                    xParent = y->get_parent();
                    if (x)
                    {
                        x->set_parent(xParent);
                    }
                    xParent->m_left = x;      // y must be a child of m_left
                    y->m_right = z->m_right;
                    z->m_right->set_parent(y);
                }
                else
                {
                    xParent = y;
                }
                base_node_ptr_type zParent = z->get_parent();
                if (m_head.get_parent() == z)
                {
                    m_head.set_parent(y);
                }
                else if (zParent->m_left == z)
                {
                    zParent->m_left = y;
                }
                else
                {
                    zParent->m_right = y;
                }
                y->set_parent(zParent);
                y->swap_color(*z);
                y = z;
                // y now points to node to be actually deleted
            }
            else
            {                        // y == z
                xParent = y->get_parent();
                if (x)
                {
                    x->set_parent(xParent);
                }

                base_node_ptr_type zParent = z->get_parent();
                if (m_head.get_parent() == z)
                {
                    m_head.set_parent(x);
                }
                else
                {
                    if (zParent->m_left == z)
                    {
                        zParent->m_left = x;
                    }
                    else
                    {
                        zParent->m_right = x;
                    }
                }

                if (leftmost == z)
                {
                    if (z->m_right == 0)        // z->m_left must be null also
                    {
                        leftmost = zParent;     // makes __leftmost == header if z == root
                    }
                    else
                    {
                        leftmost = x->minimum();
                    }
                }
                if (rightmost == z)
                {
                    if (z->m_left == 0)         // z->m_right must be null also
                    {
                        rightmost = zParent;
                    }
                    // makes __rightmost == header if z == root
                    else                      // x == z->m_left
                    {
                        rightmost = x->maximum();
                    }
                }
            }

            if (y->get_color() != AZSTD_RBTREE_RED)
            {
                while (x != m_head.get_parent() && (x == 0 || x->get_color() == AZSTD_RBTREE_BLACK))
                {
                    if (x == xParent->m_left)
                    {
                        base_node_ptr_type w = xParent->m_right;
                        if (w->get_color() == AZSTD_RBTREE_RED)
                        {
                            w->set_color(AZSTD_RBTREE_BLACK);
                            xParent->set_color(AZSTD_RBTREE_RED);
                            rotate_left(xParent);
                            w = xParent->m_right;
                        }
                        if ((w->m_left == 0 || w->m_left->get_color() == AZSTD_RBTREE_BLACK) &&
                            (w->m_right == 0 || w->m_right->get_color() == AZSTD_RBTREE_BLACK))
                        {
                            w->set_color(AZSTD_RBTREE_RED);
                            x = xParent;
                            xParent = xParent->get_parent();
                        }
                        else
                        {
                            if (w->m_right == 0 || w->m_right->get_color() == AZSTD_RBTREE_BLACK)
                            {
                                if (w->m_left)
                                {
                                    w->m_left->set_color(AZSTD_RBTREE_BLACK);
                                }
                                w->set_color(AZSTD_RBTREE_RED);
                                rotate_right(w);
                                w = xParent->m_right;
                            }
                            w->set_color(xParent->get_color());
                            xParent->set_color(AZSTD_RBTREE_BLACK);
                            if (w->m_right)
                            {
                                w->m_right->set_color(AZSTD_RBTREE_BLACK);
                            }
                            rotate_left(xParent);
                            break;
                        }
                    }
                    else
                    {
                        // same as above, with m_right <-> m_left.
                        base_node_ptr_type w = xParent->m_left;
                        if (w->get_color() == AZSTD_RBTREE_RED)
                        {
                            w->set_color(AZSTD_RBTREE_BLACK);
                            xParent->set_color(AZSTD_RBTREE_RED);
                            rotate_right(xParent);
                            w = xParent->m_left;
                        }
                        if ((w->m_right == 0 || w->m_right->get_color() == AZSTD_RBTREE_BLACK) &&
                            (w->m_left == 0 ||  w->m_left->get_color() == AZSTD_RBTREE_BLACK))
                        {
                            w->set_color(AZSTD_RBTREE_RED);
                            x = xParent;
                            xParent = xParent->get_parent();
                        }
                        else
                        {
                            if (w->m_left == 0 || w->m_left->get_color() == AZSTD_RBTREE_BLACK)
                            {
                                if (w->m_right)
                                {
                                    w->m_right->set_color(AZSTD_RBTREE_BLACK);
                                }
                                w->set_color(AZSTD_RBTREE_RED);
                                rotate_left(w);
                                w = xParent->m_left;
                            }
                            w->set_color(xParent->get_color());
                            xParent->set_color(AZSTD_RBTREE_BLACK);
                            if (w->m_left)
                            {
                                w->m_left->set_color(AZSTD_RBTREE_BLACK);
                            }
                            rotate_right(xParent);
                            break;
                        }
                    }
                }
                if (x)
                {
                    x->set_color(AZSTD_RBTREE_BLACK);
                }
            }
            return y;
        }

        Internal::rbtree_node_base  m_head;
        size_type                   m_numElements;
        key_equal                      m_keyEq;
        allocator_type              m_allocator;

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        inline void orphan_node(node_ptr_type node) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "rbtree::orphan_node - iterator was corrupted!");
                node_ptr_type nodePtr = static_cast<iterator*>(iter)->m_iter.m_node;

                if (nodePtr == node)
                {
                    // orphan the iterator
                    iter->m_container = 0;

                    if (iter->m_prevIterator)
                    {
                        iter->m_prevIterator->m_nextIterator = iter->m_nextIterator;
                    }
                    else
                    {
                        m_iteratorList = iter->m_nextIterator;
                    }

                    if (iter->m_nextIterator)
                    {
                        iter->m_nextIterator->m_prevIterator = iter->m_prevIterator;
                    }
                }

                iter = iter->m_nextIterator;
            }
        }
#endif
    };

    #undef AZSTD_RBTREE_RED
    #undef AZSTD_RBTREE_BLACK

    template <class Traits>
    template <class InsertReturnType, class NodeHandle>
    inline InsertReturnType rbtree<Traits>::node_handle_insert_unique(NodeHandle&& nodeHandle)
    {
        if (nodeHandle.empty())
        {
            return { end(), false, NodeHandle{} };
        }

        InsertReturnType result;
        pair<iterator, bool> insertResult = insert_unique_node(nodeHandle.m_node);
        result.position = insertResult.first;
        result.inserted = insertResult.second;
        if (!result.inserted)
        {
            result.node = AZStd::move(nodeHandle);
        }
        nodeHandle.release_node();

        return result;
    }

    template <class Traits>
    template <class NodeHandle>
    inline auto rbtree<Traits>::node_handle_insert_unique(const iterator hint, NodeHandle&& nodeHandle) -> iterator
    {
        if (nodeHandle.empty())
        {
            return end();
        }

        iterator insertIter = find(Traits::key_from_value(nodeHandle.m_node->m_value));
        if (insertIter == end())
        {
            insertIter = insert_unique_node(hint, nodeHandle.m_node);
            nodeHandle.release_node();
        }
        return insertIter;
    }

    template <class Traits>
    template <class NodeHandle>
    inline auto rbtree<Traits>::node_handle_insert_equal(NodeHandle&& nodeHandle) -> iterator
    {
        if (nodeHandle.empty())
        {
            return end();
        }

        iterator insertIter = insert_equal_node(nodeHandle.m_node);
        nodeHandle.release_node();
        return insertIter;
    }
    template <class Traits>
    template <class NodeHandle>
    inline auto rbtree<Traits>::node_handle_insert_equal(const iterator hint, NodeHandle&& nodeHandle) -> iterator
    {
        if (nodeHandle.empty())
        {
            return end();
        }

        iterator insertIter = insert_equal_node(hint, nodeHandle.m_node);
        nodeHandle.release_node();
        return insertIter;
    }

    template <class Traits>
    template <class NodeHandle>
    inline NodeHandle rbtree<Traits>::node_handle_extract(const key_type& key)
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
    inline NodeHandle rbtree<Traits>::node_handle_extract(const_iterator extractPos)
    {
        // Makes a non-const iterator out of a const iterator
        node_ptr_type extractNode = extractPos.m_node;
        iterator next(AZSTD_CHECKED_ITERATOR(iterator_impl, extractNode));
        ++next;

        // Remove the node from the tree, but don't delete it
        node_ptr_type nodeToExtract = static_cast<node_ptr_type>(rebalance_for_erase(extractNode, m_head.m_left, m_head.m_right));
        --m_numElements;
        nodeToExtract->set_parent(nullptr);
        nodeToExtract->m_left = nullptr;
        nodeToExtract->m_right = nullptr;

        return NodeHandle{ nodeToExtract, get_allocator() };
    }

    template <class Traits>
    template <typename ComparableToKey, typename... Args>
    auto rbtree<Traits>::try_emplace_unique(ComparableToKey&& key, Args&&... arguments) -> AZStd::pair<iterator, bool>
    {
        // Check if the key has a corresponding node in the container
        iterator insertIter = lower_bound(key);
        if (insertIter.m_node != &m_head && !m_keyEq(key, insertIter->first))
        {
            return { insertIter, false };
        }

        base_node_ptr_type newNode = create_node(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
            AZStd::forward_as_tuple(AZStd::forward<Args>(arguments)...));
        return { insert_unique_node(insertIter, newNode), true };
    }

    template <class Traits>
    template <typename ComparableToKey, typename... Args>
    auto rbtree<Traits>::try_emplace_unique(const_iterator hint, ComparableToKey&& key, Args&&... arguments) -> iterator
    {
        // Check if the key has a corresponding node in the container
        iterator insertIter = lower_bound(key);
        if (insertIter.m_node != &m_head && !m_keyEq(key, insertIter->first))
        {
            return insertIter;
        }

        base_node_ptr_type newNode = create_node(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
            AZStd::forward_as_tuple(AZStd::forward<Args>(arguments)...));
        return insert_unique_node(hint, newNode);
    }

    template <class Traits>
    template <typename ComparableToKey, typename MappedType>
    auto rbtree<Traits>::insert_or_assign_unique(ComparableToKey&& key, MappedType&& value) -> AZStd::pair<iterator, bool>
    {
        // Check if the key has a corresponding node in the container
        iterator insertIter = lower_bound(key);
        if (insertIter.m_node != &m_head && !m_keyEq(key, insertIter->first))
        {
            // Update the mapped element if the key has been found
            insertIter->second = AZStd::forward<MappedType>(value);
            return { insertIter, false };
        }

        base_node_ptr_type newNode = create_node(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
            AZStd::forward_as_tuple(AZStd::forward<MappedType>(value)));
        return { insert_unique_node(insertIter, newNode), true };
    }

    template <class Traits>
    template <typename ComparableToKey, typename MappedType>
    auto rbtree<Traits>::insert_or_assign_unique(const_iterator hint, ComparableToKey&& key, MappedType&& value) -> iterator
    {
        // Check if the key has a corresponding node in the container
        iterator insertIter = lower_bound(key);
        if (insertIter.m_node != &m_head && !m_keyEq(key, insertIter->first))
        {
            // Update the mapped element if the key has been found
            insertIter->second = AZStd::forward<MappedType>(value);
            return insertIter;
        }

        base_node_ptr_type newNode = create_node(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<ComparableToKey>(key)),
            AZStd::forward_as_tuple(AZStd::forward<MappedType>(value)));
        return insert_unique_node(hint, newNode);
    }
}

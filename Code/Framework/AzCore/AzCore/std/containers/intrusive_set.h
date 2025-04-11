/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_INTRUSIVE_SET_H
#define AZSTD_INTRUSIVE_SET_H 1

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

// Enable enable checks that validate that the set is correct on insertions and removals
//#define DEBUG_INTRUSIVE_MULTISET

namespace AZStd
{
    #define AZSTD_RBTREE_BLACK  0
    #define AZSTD_RBTREE_RED    1
    #define AZSTD_RBTREE_LEFT   0
    #define AZSTD_RBTREE_RIGHT  1

    /**
     * This is the node you need to include in your objects, if you want to use
     * it in intrusive multi set. You can do that either by inheriting it
     * or add it as a \b public member. They way you include the node should be in
     * using the appropriate hooks.
     * Check the intrusive_multi_set \ref AZStdExamples.
     */
    template< class T >
    struct intrusive_multiset_node
    {
        typedef int     ColorType;
        typedef int     SideType;

        typedef intrusive_multiset_node<T> this_type;
        typedef T*      node_ptr_type;

        template<class U, class Hook, class Compare>
        friend class intrusive_multiset;

        intrusive_multiset_node() = default;
        ~intrusive_multiset_node()
        {
#if defined(AZ_DEBUG_BUILD)
            AZSTD_CONTAINER_ASSERT(m_children[0] == nullptr && m_children[1] == nullptr && m_parentColorSide == nullptr, "AZStd::intrusive_multiset_node - intrusive list node is being destroyed while still in a container!");
#endif
        }
        // We use the lower 2 bits of the parent pointer to store
        enum Bits
        {
            BIT_COLOR = 0,          ///< Color of the node.
            BIT_PARENT_SIDE = 1,    ///< Store the side of parent Left of Right (order is up to the container)
            BIT_MASK = 3,
        };

        AZ_FORCE_INLINE ColorType  getColor() const { return static_cast<ColorType>((reinterpret_cast<AZStd::size_t>(m_parentColorSide) >> BIT_COLOR) & 1); }
        AZ_FORCE_INLINE void       setColor(ColorType color)
        {
            AZStd::size_t parentColorSide = reinterpret_cast<AZStd::size_t>(m_parentColorSide);
            parentColorSide &= ~(AZStd::size_t)(1 << BIT_COLOR);
            parentColorSide |= (AZStd::size_t)(color << BIT_COLOR);
            m_parentColorSide = reinterpret_cast<T*>(parentColorSide);
        }
        AZ_FORCE_INLINE SideType getParentSide() const { return static_cast<SideType>((reinterpret_cast<AZStd::size_t>(m_parentColorSide) >> BIT_PARENT_SIDE) & 1); }
        AZ_FORCE_INLINE void     setParentSide(SideType side)
        {
            AZStd::size_t parentColorSide = reinterpret_cast<AZStd::size_t>(m_parentColorSide);
            parentColorSide &= ~(AZStd::size_t)(1 << BIT_PARENT_SIDE);
            parentColorSide |= (AZStd::size_t)(side << BIT_PARENT_SIDE);
            m_parentColorSide = reinterpret_cast<T*>(parentColorSide);
        }
        AZ_FORCE_INLINE T*      getParent() const       { return reinterpret_cast<T*>(reinterpret_cast<AZStd::size_t>(m_parentColorSide) & ~static_cast<AZStd::size_t>(BIT_MASK)); }
        AZ_FORCE_INLINE void    setParent(T* parent)
        {
            AZSTD_CONTAINER_ASSERT((reinterpret_cast<AZStd::size_t>(parent) & BIT_MASK) == 0, "We require nodes to be at least 4 bytes aligned!");
            AZStd::size_t newParentColorSide = reinterpret_cast<AZStd::size_t>(parent);
            AZStd::size_t parentColorSide = reinterpret_cast<AZStd::size_t>(m_parentColorSide);
            parentColorSide &= BIT_MASK;
            parentColorSide |= newParentColorSide;
            m_parentColorSide = reinterpret_cast<T*>(parentColorSide);
        }

        AZ_FORCE_INLINE T*      prev() const { return m_neighbours[AZSTD_RBTREE_LEFT]; }
        AZ_FORCE_INLINE T*      next() const { return m_neighbours[AZSTD_RBTREE_RIGHT];  }

    protected:
        T* m_children[2]{};
        T* m_neighbours[2]{};
        T* m_parentColorSide{};
    } AZ_MAY_ALIAS; // we access the object in such way that we potentially can break strict aliasing rules

    /**
     * Hook when our object inherits \ref intrusive_multiset_node. So the container
     * will use the base class.
     * Check the intrusive_multi_set \ref AZStdExamples.
     */
    template< class T >
    struct intrusive_multiset_base_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_multiset_node<T>              node_type;
        typedef node_type*                              node_ptr_type;
        typedef const node_type*                        const_node_ptr_type;

        static node_ptr_type        to_node_ptr(pointer ptr)        { return ptr; }
        static const_node_ptr_type  to_node_ptr(const_pointer ptr)  { return ptr; }
    };

    /**
     * Hook for member nodes. The node should be declared public so we can access it
     * directly.
     * Check the intrusive_multi_set \ref AZStdExamples.
     */
    template< class T, intrusive_multiset_node<T> T::* PtrToMember >
    struct intrusive_multiset_member_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_multiset_node<T>              node_type;
        typedef node_type*                              node_ptr_type;
        typedef const node_type*                        const_node_ptr_type;

        static node_ptr_type    to_node_ptr(pointer ptr)            { return static_cast<node_ptr_type>(&(ptr->*PtrToMember)); }
        static const_node_ptr_type to_node_ptr(const_pointer ptr)   { return static_cast<const_node_ptr_type>(&(ptr->*PtrToMember)); }
    };

    /**
     * Intrusive multiset container is a AZStd extension container.
     * \par
     * It uses the \ref intrusive_multiset_node contained in the provided user type T.
     * You have 2 choices you can make your T type to either inherit from \ref intrusive_multiset_node or just have intrusive_multiset_node
     * as a \b public member. Based on which option you prefer you should use the appropriate hook type.
     * Hook parameter should be intrusive_multiset_base_hook (if you inherit the \ref intrusive_multiset_node) or intrusive_multiset_member_hook if you have it as a public member.
     * Intrusive containers never allocate any memory, destroy or create any objects. Just used the provided nodes to store it's information.
     * Generally speaking intrusive containers are better for the cache (assuming you are operating on the objects you are iterating)
     * You can see examples of all that \ref AZStdExamples.
     */
    template< class T, class Hook, class Compare = AZStd::less<T> >
    class intrusive_multiset
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef intrusive_multiset<T, Hook, Compare>    this_type;
        typedef int                             SideType;

        friend class const_iterator_impl;
        friend class iterator_impl;
    public:
        typedef T*                              pointer;
        typedef const T*                        const_pointer;

        typedef T&                              reference;
        typedef const T&                        const_reference;
        typedef typename AZStd::size_t          difference_type;
        typedef typename AZStd::size_t          size_type;

        typedef T                               KeyType;
        typedef Compare                         KeyCompare;

        // AZSTD extension.
        typedef T                               value_type;
        typedef T                               node_type;
        typedef node_type*                      node_ptr_type;
        typedef const node_type*                const_node_ptr_type;
        typedef intrusive_multiset_node<T>      hook_node_type;
        typedef hook_node_type*                 hook_node_ptr_type;

        /**
         * Constant iterator implementation bidirectional iterator.
         */
        class const_iterator_impl
        {
            enum
            {
                ITERATOR_VERSION = 1
            };

            friend class intrusive_multiset;
            typedef const_iterator_impl         this_type;
            typedef intrusive_multiset<T, Hook> tree_type;
        public:
            typedef const T                     value_type;
            typedef AZStd::ptrdiff_t            difference_type;
            typedef const T*                    pointer;
            typedef const T&                    reference;
            typedef bidirectional_iterator_tag  iterator_category;


            AZ_FORCE_INLINE const_iterator_impl()
                : m_node(nullptr) {}
            AZ_FORCE_INLINE const_iterator_impl(const_node_ptr_type node)
                : m_node(const_cast<node_ptr_type>(node)) {}
            AZ_FORCE_INLINE const_reference operator*() const   { return *m_node; }
            AZ_FORCE_INLINE pointer operator->() const  { return m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                m_node = PredOrSucc(m_node, AZSTD_RBTREE_RIGHT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = PredOrSucc(m_node, AZSTD_RBTREE_RIGHT);
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                m_node = PredOrSucc(m_node, AZSTD_RBTREE_LEFT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = PredOrSucc(m_node, AZSTD_RBTREE_LEFT);
                return temp;
            }

            AZ_FORCE_INLINE bool operator==(const this_type& rhs) const { return (m_node == rhs.m_node); }
            AZ_FORCE_INLINE bool operator!=(const this_type& rhs) const { return (m_node != rhs.m_node); }
        protected:
            AZ_FORCE_INLINE static bool isNil(const_node_ptr_type node) { return node == Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_RIGHT]; }

            inline static node_ptr_type PredOrSucc(const_node_ptr_type node, SideType side)
            {
                node_ptr_type cur = Hook::to_node_ptr(node)->m_neighbours[side];
                hook_node_ptr_type curHook = Hook::to_node_ptr(cur);
                if (!curHook->getParent())
                {
                    return cur;
                }
                node_ptr_type xessor = curHook->m_children[side];
                if (!isNil(xessor))
                {
                    SideType o = (SideType)(1 - side);
                    while (!isNil(Hook::to_node_ptr(xessor)->m_children[o]))
                    {
                        xessor = Hook::to_node_ptr(xessor)->m_children[o];
                    }
                }
                else
                {
                    AZSTD_CONTAINER_ASSERT(!isNil(cur), "This node can't be nil");
                    xessor = curHook->getParent();
                    while (!isNil(xessor) && curHook->getParentSide() == side)
                    {
                        cur = xessor;
                        curHook = Hook::to_node_ptr(cur);
                        xessor = Hook::to_node_ptr(xessor)->getParent();
                    }
                }
                return xessor;
            }

            node_ptr_type   m_node;
        };

        /**
         * Iterator implementation.
         */
        class iterator_impl
            : public const_iterator_impl
        {
            typedef iterator_impl       this_type;
            typedef const_iterator_impl base_type;
        public:
            typedef T                               value_type;
            typedef T*                              pointer;
            typedef T&                              reference;

            AZ_FORCE_INLINE iterator_impl() {}
            AZ_FORCE_INLINE iterator_impl(node_ptr_type node)
                : const_iterator_impl(node) {}
            AZ_FORCE_INLINE reference operator*() const { return *base_type::m_node; }
            AZ_FORCE_INLINE pointer operator->() const { return base_type::m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                base_type::m_node = base_type::PredOrSucc(base_type::m_node, AZSTD_RBTREE_RIGHT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                this_type temp = *this;
                base_type::m_node = base_type::PredOrSucc(base_type::m_node, AZSTD_RBTREE_RIGHT);
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                base_type::m_node = base_type::PredOrSucc(base_type::m_node, AZSTD_RBTREE_LEFT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                this_type temp = *this;
                base_type::m_node = base_type::PredOrSucc(base_type::m_node, AZSTD_RBTREE_LEFT);
                return temp;
            }
        };

        /**
         * reverse iterator implementation bidirectional iterator.
         */
        template<typename Iter>
        class reverse_iterator_impl
        {
            friend class intrusive_multiset;
            typedef reverse_iterator_impl this_type;
            typedef intrusive_multiset<T, Hook> tree_type;
        public:
            using value_type = typename AZStd::iterator_traits<Iter>::value_type;
            using difference_type = typename AZStd::iterator_traits<Iter>::difference_type;
            using pointer = typename AZStd::iterator_traits<Iter>::pointer;
            using reference = typename AZStd::iterator_traits<Iter>::reference;
            using iterator_category = typename AZStd::iterator_traits<Iter>::iterator_category;
            using iterator_type = Iter;

            AZ_FORCE_INLINE reverse_iterator_impl()
                : m_node(nullptr) {}
            AZ_FORCE_INLINE reverse_iterator_impl(pointer node, const value_type* head)
                : m_node(node)
                , m_headNode(head)
            {}

            iterator_type base() const
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_set::reference_iterator_impl invoked base() on invalid node!");
                // The intrusive multiset head node is a fake head node that represents a sentinel node
                // Therefore in the reverse iterator case, when the head node is stored in the iterator the minimum value needs to be returned
                // otherwise the successor value needs to be returned
                return m_node == m_headNode
                    ? MinOrMax(Hook::to_node_ptr(m_node)->m_children[AZSTD_RBTREE_LEFT], AZSTD_RBTREE_LEFT)
                    : iterator_type(GetPredicateOrSuccessorNode(m_node, AZSTD_RBTREE_RIGHT));
            }
            AZ_FORCE_INLINE reference operator*() const { return *m_node; }
            AZ_FORCE_INLINE pointer operator->() const { return m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_set::reference_iterator_impl invalid node!");
                m_node = GetPredicateOrSuccessorNode(m_node, AZSTD_RBTREE_LEFT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_set::reference_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = GetPredicateOrSuccessorNode(m_node, AZSTD_RBTREE_LEFT);
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_set::reference_iterator_impl invalid node!");
                m_node = GetPredicateOrSuccessorNode(m_node, AZSTD_RBTREE_RIGHT);
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_set::reference_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = GetPredicateOrSuccessorNode(m_node, AZSTD_RBTREE_RIGHT);
                return temp;
            }

            AZ_FORCE_INLINE bool operator==(const this_type& rhs) const { return (m_node == rhs.m_node); }
            AZ_FORCE_INLINE bool operator!=(const this_type& rhs) const { return (m_node != rhs.m_node); }
        protected:

            static bool IsNilLeafNode(const_node_ptr_type node) { return node == Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_RIGHT]; }

            static node_ptr_type GetPredicateOrSuccessorNode(const_node_ptr_type node, SideType childNodeSide)
            {
                node_ptr_type cur = Hook::to_node_ptr(node)->m_neighbours[childNodeSide];
                hook_node_ptr_type curHook = Hook::to_node_ptr(cur);
                if (!curHook->getParent())
                {
                    return cur;
                }
                node_ptr_type predicateOrSuccessorNode = curHook->m_children[childNodeSide];
                if (!IsNilLeafNode(predicateOrSuccessorNode))
                {
                    SideType otherChildNodeSide = (SideType)(1 - childNodeSide);
                    while (!IsNilLeafNode(Hook::to_node_ptr(predicateOrSuccessorNode)->m_children[otherChildNodeSide]))
                    {
                        predicateOrSuccessorNode = Hook::to_node_ptr(predicateOrSuccessorNode)->m_children[otherChildNodeSide];
                    }
                }
                else
                {
                    AZSTD_CONTAINER_ASSERT(!IsNilLeafNode(cur), "This node can't be nil");
                    predicateOrSuccessorNode = curHook->getParent();
                    while (!IsNilLeafNode(predicateOrSuccessorNode) && curHook->getParentSide() == childNodeSide)
                    {
                        cur = predicateOrSuccessorNode;
                        curHook = Hook::to_node_ptr(cur);
                        predicateOrSuccessorNode = Hook::to_node_ptr(predicateOrSuccessorNode)->getParent();
                    }
                }
                return predicateOrSuccessorNode;
            }

            pointer m_node;
            const value_type* m_headNode{};
        };

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_bidirectional_iterator<iterator_impl, this_type>                 iterator;
        typedef Debug::checked_bidirectional_iterator<const_iterator_impl, this_type>           const_iterator;
        typedef Debug::checked_bidirectional_iterator<reverse_iterator_impl<iterator>, this_type>         reverse_iterator;
        typedef Debug::checked_bidirectional_iterator<reverse_iterator_impl<const_iterator>, this_type>   const_reverse_iterator;
#else
        typedef iterator_impl                   iterator;
        typedef const_iterator_impl             const_iterator;
        typedef reverse_iterator_impl<iterator>           reverse_iterator;
        typedef reverse_iterator_impl<const_iterator>     const_reverse_iterator;
#endif

        AZ_FORCE_INLINE explicit intrusive_multiset()
            : m_numElements(0)
        {
            setNil(get_head());
        }

        AZ_FORCE_INLINE intrusive_multiset(const KeyCompare& keyEq)
            : m_numElements(0)
            , m_keyCompare(keyEq)
        {
            setNil(get_head());
        }
        template<class InputIterator>
        AZ_FORCE_INLINE intrusive_multiset(const InputIterator& first, const InputIterator& last)
            : m_numElements(0)
        {
            setNil(get_head());
            insert(first, last);
        }

        ~intrusive_multiset()
        {
            clear();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // We can completely forbid copy/assign operators once we fully switch for containers that require movable only.
        AZ_FORCE_INLINE intrusive_multiset(const this_type& rhs)
        {
            (void)rhs;
            AZSTD_CONTAINER_ASSERT(rhs.empty(), "You can't copy an intrusive linked list, use move semantics!");
            setNil(get_head());
            m_numElements = 0;
        }
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            (void)rhs;
            AZSTD_CONTAINER_ASSERT(rhs.empty(), "You can't copy an intrusive linked list, use move semantics!");
            setNil(get_head());
            m_numElements = 0;
            return *this;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        AZ_FORCE_INLINE const_iterator begin() const { return minimum(); }
        AZ_FORCE_INLINE iterator begin() { return minimum(); }
        AZ_FORCE_INLINE const_iterator end() const { return const_iterator(get_head()); }
        AZ_FORCE_INLINE iterator end() { return iterator(get_head()); }

        AZ_FORCE_INLINE reverse_iterator rbegin()               { return reverse_iterator(MinOrMax(get_root(), AZSTD_RBTREE_RIGHT), get_head()); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const   { return const_reverse_iterator(MinOrMax(get_root(), AZSTD_RBTREE_RIGHT), get_head()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(MinOrMax(get_root(), AZSTD_RBTREE_RIGHT), get_head()); }
        AZ_FORCE_INLINE reverse_iterator rend()                 { return reverse_iterator(get_head(), get_head()); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const     { return const_reverse_iterator(get_head(), get_head()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(get_head(), get_head()); }

        AZ_FORCE_INLINE const_iterator root() const { return const_iterator(get_root()); }
        AZ_FORCE_INLINE iterator root() { return iterator(get_root()); }
        AZ_FORCE_INLINE const_iterator minimum() const { return const_iterator(MinOrMax(get_root(), AZSTD_RBTREE_LEFT)); }
        AZ_FORCE_INLINE iterator minimum() { return iterator(MinOrMax(get_root(), AZSTD_RBTREE_LEFT)); }
        AZ_FORCE_INLINE const_iterator maximum() const { return const_iterator(MinOrMax(get_root(), AZSTD_RBTREE_RIGHT)); }
        AZ_FORCE_INLINE iterator maximum() { return iterator(MinOrMax(get_root(), AZSTD_RBTREE_RIGHT)); }

        AZ_FORCE_INLINE iterator lower_bound(const KeyType& key) { return iterator(DoLowerBound(key)); }
        AZ_FORCE_INLINE const_iterator lower_bound(const KeyType& key) const { return const_iterator(DoLowerBound(key)); }
        template<class ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, iterator> lower_bound(const ComparableToKey& key) { return iterator(DoLowerBound(key)); }
        template<class ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, const_iterator> lower_bound(const ComparableToKey& key) const { return const_iterator(DoLowerBound(key)); }

        AZ_FORCE_INLINE iterator upper_bound(const KeyType& key) { return iterator(DoUpperBound(key)); }
        AZ_FORCE_INLINE const_iterator upper_bound(const KeyType& key) const { return const_iterator(DoUpperBound(key)); }
        template<class ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, iterator> upper_bound(const ComparableToKey& key) { return iterator(DoUpperBound(key)); }
        template<class ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, const_iterator> upper_bound(const ComparableToKey& key) const { return const_iterator(DoUpperBound(key)); }
        AZ_FORCE_INLINE iterator find(const KeyType& key)
        {
            T* found = DoLowerBound(key);
            if (found == get_head() || m_keyCompare(key, *found))
            {
                return end();
            }
            return iterator(found);
        }
        AZ_FORCE_INLINE const_iterator find(const KeyType& key) const
        {
            const T* found = DoLowerBound(key);
            if (found == get_head() || m_keyCompare(key, *found))
            {
                return end();
            }
            return const_iterator(found);
        }

        iterator    insert(T* value)
        {
            //#ifdef AZ_DEBUG_BUILD
            //          hook_node_ptr_type dbgValueHook = Hook::to_node_ptr(value);
            //          (void)dbgValueHook;
            //          AZSTD_CONTAINER_ASSERT(dbgValueHook->m_children[0] == nullptr && dbgValueHook->m_children[1] == nullptr && dbgValueHook->m_parentColorSide == nullptr,
            //              "This node is already attached to the list");
            //#endif
            node_ptr_type node = value;
            node_ptr_type endNode = get_head();
            node_ptr_type lastNode = get_head();
            node_ptr_type curNode = get_root();
            SideType s = AZSTD_RBTREE_LEFT;
            ++m_numElements;
            while (curNode != endNode)
            {
                lastNode = curNode;
                s = AZSTD_RBTREE_RIGHT;
                if (m_keyCompare(*node, *curNode))
                {
                    s = AZSTD_RBTREE_LEFT;
                }
                else if (!m_keyCompare(*curNode, *node))
                {
                    Link(node, curNode);
                    return iterator(node);
                }
                curNode = Hook::to_node_ptr(curNode)->m_children[s];
            }
            AttachTo(node, lastNode, s);
            InsertFixup(node);

#ifdef DEBUG_INTRUSIVE_MULTISET
            validate();
#endif
            return iterator(node);
        }

        AZ_FORCE_INLINE iterator insert(T& value)
        {
            return insert(&value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void insert(InputIterator first, InputIterator last)
        {
            while (first != last)
            {
                insert(*first++);
            }
        }

        void        erase(T* value)
        {
            node_ptr_type node = value;
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);

#ifdef AZ_DEBUG_BUILD
            // TODO: we can add slow debug check to make sure the element is on the container
            AZSTD_CONTAINER_ASSERT(nodeHook->m_children[0] != nullptr || nodeHook->m_children[1] != nullptr || nodeHook->m_parentColorSide != nullptr ||
                nodeHook->m_neighbours[0] != nullptr || nodeHook->m_neighbours[1] != nullptr, "This node is NOT attached to this list");
#endif
            if (nodeHook->m_neighbours[AZSTD_RBTREE_LEFT] != node)
            {
                if (nodeHook->getParent() == nullptr)
                {
                    AZSTD_CONTAINER_ASSERT(nodeHook->m_children[AZSTD_RBTREE_LEFT] == nullptr, "");
                    AZSTD_CONTAINER_ASSERT(nodeHook->m_children[AZSTD_RBTREE_RIGHT] == nullptr, "");
                    Unlink(node);
                }
                else
                {
                    T* repl = nodeHook->m_neighbours[AZSTD_RBTREE_RIGHT];
                    AZSTD_CONTAINER_ASSERT(repl != get_head(), "");
                    AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(repl)->getParent() == nullptr, "");
                    AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(repl)->m_children[AZSTD_RBTREE_LEFT] == nullptr, "");
                    AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(repl)->m_children[AZSTD_RBTREE_RIGHT] == nullptr, "");
                    Switch(repl, node);
                    Unlink(node);
                }
            }
            else
            {
                T* endNode = get_head();
                T* repl = node;
                hook_node_ptr_type replHook = Hook::to_node_ptr(repl);
                SideType s = AZSTD_RBTREE_LEFT;
                if (nodeHook->m_children[AZSTD_RBTREE_RIGHT] != endNode)
                {
                    if (nodeHook->m_children[AZSTD_RBTREE_LEFT] != endNode)
                    {
                        repl = nodeHook->m_children[AZSTD_RBTREE_RIGHT];
                        replHook = Hook::to_node_ptr(repl);
                        while (replHook->m_children[AZSTD_RBTREE_LEFT] != endNode)
                        {
                            repl = replHook->m_children[AZSTD_RBTREE_LEFT];
                            replHook = Hook::to_node_ptr(repl);
                        }
                    }
                    s = AZSTD_RBTREE_RIGHT;
                }
                AZSTD_CONTAINER_ASSERT(replHook->m_children[1 - s] == endNode, "");
                bool red = replHook->getColor() == AZSTD_RBTREE_RED;
                T* replChild = replHook->m_children[s];
                Substitute(repl, replChild);
                if (repl != node)
                {
                    Switch(repl, node);
                }
                if (!red)
                {
                    EraseFixup(replChild);
                }
            }

            --m_numElements;

            nodeHook->m_children[0] = nodeHook->m_children[1] = nullptr;
            nodeHook->m_parentColorSide = nullptr;
#if defined(AZ_DEBUG_BUILD) && defined(DEBUG_INTRUSIVE_MULTISET)
            validate();
#endif
        }

        AZ_FORCE_INLINE void        erase(T& value)
        {
            erase(&value);
        }

        AZ_FORCE_INLINE iterator    erase(const_iterator where)
        {
            T* node = const_cast<T*>(&*where);
            ++where;
            erase(node);
            return AZStd::Internal::ConstIteratorCast<iterator>(where);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void        erase(InputIterator first, InputIterator last)
        {
            while (first != last)
            {
                erase(*first++);
            }
        }

        AZ_FORCE_INLINE void        clear()
        {
            for (iterator it = begin(); it != end(); )
            {
                it = erase(it);
            }
        }

        AZ_FORCE_INLINE AZStd::size_t size() const { return m_numElements; }

        AZ_FORCE_INLINE bool empty() const { return get_root() == get_head(); }

#ifdef DEBUG_INTRUSIVE_MULTISET
        inline unsigned int validateHeight(const_node_ptr_type node) const
        {
            if (node == get_head())
            {
                return 0;
            }
            if (Hook::to_node_ptr(node)->getColor() == AZSTD_RBTREE_BLACK)
            {
                return validateHeight(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_LEFT]) + validateHeight(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_RIGHT]) + 1;
            }
            AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_LEFT])->getColor() == AZSTD_RBTREE_BLACK
                && Hook::to_node_ptr(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_RIGHT])->getColor() == AZSTD_RBTREE_BLACK, "");
            const unsigned lh = validateHeight(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_LEFT]);
            const unsigned rh = validateHeight(Hook::to_node_ptr(node)->m_children[AZSTD_RBTREE_RIGHT]);
            AZSTD_CONTAINER_ASSERT(lh == rh, "");
            return lh;
        }

        // Validate container status.
        inline bool validate() const
        {
            AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(get_head())->getColor() == AZSTD_RBTREE_BLACK, "");
            AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_RIGHT] == get_head(), "");
            AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_LEFT] == get_head() || Hook::to_node_ptr(Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_LEFT])->getColor() == AZSTD_RBTREE_BLACK, "");
            validateHeight(Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_LEFT]);
            return true;
        }
#endif

        // Validate iterator.
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            node_ptr_type iterNode = iter.m_iter.m_node;
#else
            node_ptr_type iterNode = iter.m_node;
#endif

            if (iterNode == get_head())
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
            if (iterNode == get_head())
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        /// Reset the container without going to unhook any elements.
        inline void     leak_and_reset()
        {
            setNil(get_head());
            m_numElements = 0;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

    protected:
        node_ptr_type DoLowerBound(const KeyType& key)
        {
            node_ptr_type endNode = get_head();
            node_ptr_type bestNode = get_head();
            node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(*curNode, key))
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
                else
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
            }
            return bestNode;
        }

        const_node_ptr_type DoLowerBound(const KeyType& key) const
        {
            const_node_ptr_type endNode = get_head();
            const_node_ptr_type bestNode = get_head();
            const_node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(*curNode, key))
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
                else
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
            }
            return bestNode;
        }

        template<typename ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, node_ptr_type> DoLowerBound(const ComparableToKey& key)
        {
            node_ptr_type endNode = get_head();
            node_ptr_type bestNode = get_head();
            node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(*curNode, key))
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
                else
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
            }
            return bestNode;
        }

        template<typename ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, const_node_ptr_type> DoLowerBound(const ComparableToKey& key) const
        {
            return const_cast<const_node_ptr_type>(const_cast<intrusive_multiset*>(this)->DoLowerBound(key));
        }

        node_ptr_type DoUpperBound(const KeyType& key)
        {
            node_ptr_type endNode = get_head();
            node_ptr_type bestNode = get_head();
            node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(key, *curNode))
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
                else
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
            }
            return bestNode;
        }

        const_node_ptr_type DoUpperBound(const KeyType& key) const
        {
            const_node_ptr_type endNode = get_head();
            const_node_ptr_type bestNode = get_head();
            const_node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(key, *curNode))
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
                else
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
            }
            return bestNode;
        }

        template<typename ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, node_ptr_type> DoUpperBound(const ComparableToKey& key)
        {
            node_ptr_type endNode = get_head();
            node_ptr_type bestNode = get_head();
            node_ptr_type curNode = get_root();
            while (curNode != endNode)
            {
                if (m_keyCompare(key, *curNode))
                {
                    bestNode = curNode;
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_LEFT];
                }
                else
                {
                    curNode = Hook::to_node_ptr(curNode)->m_children[AZSTD_RBTREE_RIGHT];
                }
            }
            return bestNode;
        }

        template<typename ComparableToKey>
        enable_if_t<Internal::is_transparent<Compare, ComparableToKey>::value, const_node_ptr_type> DoUpperBound(const ComparableToKey& key) const
        {
            return const_cast<const_node_ptr_type>(const_cast<intrusive_multiset*>(this)->DoUpperBound(key));
        }

        inline void setNil(node_ptr_type node) const
        {
            hook_node_ptr_type hookNode = Hook::to_node_ptr(node);
            hookNode->m_children[0] = node;
            hookNode->m_children[1] = node;
            hookNode->m_neighbours[0] = node;
            hookNode->m_neighbours[1] = node;
            hookNode->m_parentColorSide = node;
        }

        inline void AttachTo(node_ptr_type node, node_ptr_type parent, SideType s) const
        {
            hook_node_ptr_type hookNode = Hook::to_node_ptr(node);
            hook_node_ptr_type hookParent = Hook::to_node_ptr(parent);
            hookNode->m_neighbours[AZSTD_RBTREE_LEFT] = node;
            hookNode->m_neighbours[AZSTD_RBTREE_RIGHT] = node;
            hookNode->m_children[AZSTD_RBTREE_LEFT] = hookParent->m_children[s];
            hookNode->m_children[AZSTD_RBTREE_RIGHT] = hookParent->m_children[s];
            hookNode->setParent(parent);
            hookNode->setParentSide(s);
            hookParent->m_children[s] = node;
            hookNode->setColor(AZSTD_RBTREE_RED);
        }

        inline void Rotate(node_ptr_type node, SideType side) const
        {
            hook_node_ptr_type hookNode = Hook::to_node_ptr(node);
            AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(hookNode->getParent())->m_children[hookNode->getParentSide()] == node, "Invalid node structure");
            SideType o = static_cast<SideType>(1 - side);
            SideType ps = hookNode->getParentSide();
            node_ptr_type top = hookNode->m_children[o];
            hook_node_ptr_type topHook = Hook::to_node_ptr(top);
            hookNode->m_children[o] = topHook->m_children[side];
            hook_node_ptr_type childHook = Hook::to_node_ptr(hookNode->m_children[o]);
            childHook->setParent(node);
            childHook->setParentSide(o);
            node_ptr_type parent = hookNode->getParent();
            hook_node_ptr_type parentHook = Hook::to_node_ptr(parent);
            topHook->setParent(parent);
            topHook->setParentSide(ps);
            parentHook->m_children[ps] = top;
            topHook->m_children[side] = node;
            hookNode->setParent(top);
            hookNode->setParentSide(side);
        }

        //! pre-condition
        //! @param node must be non-nullptr
        inline static node_ptr_type MinOrMax(const_node_ptr_type node, SideType side)
        {
            AZSTD_CONTAINER_ASSERT(node != nullptr, "MinOrMax can only be called with a non-nullptr node")
            const_node_ptr_type cur = node;
            const_node_ptr_type minMax = cur;
            while (!iterator_impl::isNil(cur))
            {
                minMax = cur;
                cur = Hook::to_node_ptr(cur)->m_children[side];
            }

            return const_cast<node_ptr_type>(minMax);
        }

        inline void Substitute(node_ptr_type node, node_ptr_type child) const
        {
            SideType ps = Hook::to_node_ptr(node)->getParentSide();
            node_ptr_type parent = Hook::to_node_ptr(node)->getParent();
            Hook::to_node_ptr(child)->setParent(parent);
            Hook::to_node_ptr(child)->setParentSide(ps);
            Hook::to_node_ptr(parent)->m_children[ps] = child;
        }

        inline void Switch(node_ptr_type node1, node_ptr_type node2) const
        {
            AZSTD_CONTAINER_ASSERT(node1 != node2, "Nodes must be different");
            hook_node_ptr_type node1Hook = Hook::to_node_ptr(node1);
            hook_node_ptr_type node2Hook = Hook::to_node_ptr(node2);
            AZSTD_CONTAINER_ASSERT(node2Hook->getParent() != nullptr, "It must be the head");
            SideType nps = node2Hook->getParentSide();
            node1Hook->m_children[AZSTD_RBTREE_LEFT] = node2Hook->m_children[AZSTD_RBTREE_LEFT];
            node1Hook->m_children[AZSTD_RBTREE_RIGHT] = node2Hook->m_children[AZSTD_RBTREE_RIGHT];
            node1Hook->setParent(node2Hook->getParent());
            node1Hook->setParentSide(nps);
            Hook::to_node_ptr(node2Hook->m_children[AZSTD_RBTREE_LEFT])->setParent(node1);
            Hook::to_node_ptr(node2Hook->m_children[AZSTD_RBTREE_LEFT])->setParentSide(AZSTD_RBTREE_LEFT);
            Hook::to_node_ptr(node2Hook->m_children[AZSTD_RBTREE_RIGHT])->setParent(node1);
            Hook::to_node_ptr(node2Hook->m_children[AZSTD_RBTREE_RIGHT])->setParentSide(AZSTD_RBTREE_RIGHT);
            Hook::to_node_ptr(node2Hook->getParent())->m_children[nps] = node1;
            node1Hook->setColor(node2Hook->getColor());
        }

        inline void Link(node_ptr_type node1, node_ptr_type node2) const
        {
            hook_node_ptr_type node1Hook = Hook::to_node_ptr(node1);
            node1Hook->m_neighbours[AZSTD_RBTREE_LEFT] = Hook::to_node_ptr(node2)->m_neighbours[AZSTD_RBTREE_LEFT];
            node1Hook->m_neighbours[AZSTD_RBTREE_RIGHT] = node2;
            Hook::to_node_ptr(node1Hook->m_neighbours[AZSTD_RBTREE_RIGHT])->m_neighbours[AZSTD_RBTREE_LEFT] = node1;
            Hook::to_node_ptr(node1Hook->m_neighbours[AZSTD_RBTREE_LEFT])->m_neighbours[AZSTD_RBTREE_RIGHT] = node1;
            node1Hook->m_children[AZSTD_RBTREE_LEFT] = nullptr;
            node1Hook->m_children[AZSTD_RBTREE_RIGHT] = nullptr;
            node1Hook->setParent(nullptr);
            node1Hook->setParentSide(AZSTD_RBTREE_LEFT);
            node1Hook->setColor(AZSTD_RBTREE_RED);
        }

        inline void Unlink(node_ptr_type node) const
        {
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
            Hook::to_node_ptr(nodeHook->m_neighbours[AZSTD_RBTREE_RIGHT])->m_neighbours[AZSTD_RBTREE_LEFT] = nodeHook->m_neighbours[AZSTD_RBTREE_LEFT];
            Hook::to_node_ptr(nodeHook->m_neighbours[AZSTD_RBTREE_LEFT])->m_neighbours[AZSTD_RBTREE_RIGHT] = nodeHook->m_neighbours[AZSTD_RBTREE_RIGHT];
        }

        void InsertFixup(node_ptr_type node)
        {
            node_ptr_type cur = node;
            node_ptr_type p = Hook::to_node_ptr(cur)->getParent();
            hook_node_ptr_type parentHook = Hook::to_node_ptr(p);
            while (parentHook->getColor() == AZSTD_RBTREE_RED)
            {
                node_ptr_type pp = parentHook->getParent();
                AZSTD_CONTAINER_ASSERT(pp != get_head(), "This should not happen");
                SideType s = parentHook->getParentSide();
                SideType o = (SideType)(1 - s);
                hook_node_ptr_type ppHook = Hook::to_node_ptr(pp);
                node_ptr_type pp_right = ppHook->m_children[o];
                hook_node_ptr_type ppRightHook = Hook::to_node_ptr(pp_right);
                if (ppRightHook->getColor() == AZSTD_RBTREE_RED)
                {
                    parentHook->setColor(AZSTD_RBTREE_BLACK);
                    ppRightHook->setColor(AZSTD_RBTREE_BLACK);
                    ppHook->setColor(AZSTD_RBTREE_RED);
                    cur = pp;
                    p = Hook::to_node_ptr(cur)->getParent();
                    parentHook = Hook::to_node_ptr(p);
                }
                else
                {
                    if (cur == parentHook->m_children[o])
                    {
                        cur = p;
                        Rotate(cur, s);
                        p = Hook::to_node_ptr(cur)->getParent();
                        parentHook = Hook::to_node_ptr(p);
                    }
                    parentHook->setColor(AZSTD_RBTREE_BLACK);
                    ppHook->setColor(AZSTD_RBTREE_RED);
                    Rotate(pp, o);
                }
            }

            Hook::to_node_ptr(get_root())->setColor(AZSTD_RBTREE_BLACK);
        }

        void EraseFixup(node_ptr_type node)
        {
            node_ptr_type cur = node;
            hook_node_ptr_type curHook = Hook::to_node_ptr(cur);
            while (curHook->getColor() != AZSTD_RBTREE_RED && cur != get_root())
            {
                node_ptr_type p = curHook->getParent();
                hook_node_ptr_type parentHook = Hook::to_node_ptr(p);
                SideType s = curHook->getParentSide();
                SideType o = (SideType)(1 - s);
                node_ptr_type w = parentHook->m_children[o];
                AZSTD_CONTAINER_ASSERT(w != get_head(), "This should not happen");
                hook_node_ptr_type wHook = Hook::to_node_ptr(w);
                if (wHook->getColor() == AZSTD_RBTREE_RED)
                {
                    AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(wHook->m_children[AZSTD_RBTREE_LEFT])->getColor() == AZSTD_RBTREE_BLACK &&
                        Hook::to_node_ptr(wHook->m_children[AZSTD_RBTREE_RIGHT])->getColor() == AZSTD_RBTREE_BLACK, "Invalid tree");
                    wHook->setColor(AZSTD_RBTREE_BLACK);
                    parentHook->setColor(AZSTD_RBTREE_RED);
                    w = wHook->m_children[s];
                    AZSTD_CONTAINER_ASSERT(w != get_head(), "This should not happen");
                    wHook = Hook::to_node_ptr(w);
                    Rotate(p, s);
                }
                if (Hook::to_node_ptr(wHook->m_children[AZSTD_RBTREE_LEFT])->getColor() == AZSTD_RBTREE_BLACK &&
                    Hook::to_node_ptr(wHook->m_children[AZSTD_RBTREE_RIGHT])->getColor() == AZSTD_RBTREE_BLACK)
                {
                    wHook->setColor(AZSTD_RBTREE_RED);
                    cur = p;
                    curHook = Hook::to_node_ptr(cur);
                }
                else
                {
                    if (Hook::to_node_ptr(wHook->m_children[o])->getColor() == AZSTD_RBTREE_BLACK)
                    {
                        Hook::to_node_ptr(wHook->m_children[s])->setColor(AZSTD_RBTREE_BLACK);
                        wHook->setColor(AZSTD_RBTREE_RED);
                        node_ptr_type c = wHook->m_children[s];
                        Rotate(w, o);
                        w = c;
                        wHook = Hook::to_node_ptr(w);
                        AZSTD_CONTAINER_ASSERT(w != get_head(), "This should not happen");
                    }
                    AZSTD_CONTAINER_ASSERT(Hook::to_node_ptr(wHook->m_children[o])->getColor() == AZSTD_RBTREE_RED, "This should not happen");
                    wHook->setColor(parentHook->getColor());
                    parentHook->setColor(AZSTD_RBTREE_BLACK);
                    Hook::to_node_ptr(wHook->m_children[o])->setColor(AZSTD_RBTREE_BLACK);
                    Rotate(p, s);
                    cur = get_root();
                    curHook = Hook::to_node_ptr(cur);
                }
            }
            curHook->setColor(AZSTD_RBTREE_BLACK);
        }

        /**
         * OK we face different problems when we implement intrusive list. We choose to create fake node_type as a head node!
         * Why ? There are 2 general ways to solve this issue:
         * 1. Store the head and tail pointer here and store pointer to the list in the iterator. This makes iterator 2 size,
         * plus operators -- and ++ include an if. This is the case because we need end iterator (which will be NULL). And we can't move it.
         * 2. We can store the base class in the iterator, and every time we need the value we compute the hook offset and recast the pointer.
         * all of this is fine, but we need to have a "simple" way to debug our containers" in a easy way.
         * So at this stage we consider the wasting a memory for a fake head node as the best solution, while we can debug the container.
         * This can change internally at any moment if needed, no interface change will occur.
         */
        typename aligned_storage<sizeof(node_type), alignment_of<node_type>::value>::type m_head;

        AZStd::size_t   m_numElements;
        KeyCompare      m_keyCompare;


        AZ_FORCE_INLINE node_ptr_type get_head() { return reinterpret_cast<node_ptr_type>(&m_head); }
        AZ_FORCE_INLINE node_ptr_type get_root() { return Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_LEFT]; }

        AZ_FORCE_INLINE const_node_ptr_type get_head() const { return reinterpret_cast<const_node_ptr_type>(&m_head); }
        AZ_FORCE_INLINE const_node_ptr_type get_root() const { return Hook::to_node_ptr(get_head())->m_children[AZSTD_RBTREE_LEFT]; }

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
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "list::orphan_node - iterator was corrupted!");
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

    template< class T, class Hook, class Compare >
    AZ_FORCE_INLINE bool operator==(const intrusive_multiset<T, Hook, Compare>& left, const intrusive_multiset<T, Hook, Compare>& right)
    {
        return &left == &right;
    }

    template< class T, class Hook, class Compare >
    AZ_FORCE_INLINE bool operator!=(const intrusive_multiset<T, Hook, Compare>& left, const intrusive_multiset<T, Hook, Compare>& right)
    {
        return !(&left == &right);
    }

    /**
     * TODO: Add intrusive_set container based on the container above (it's node should be -2 pointers for neighbours)
     */



    #undef AZSTD_RBTREE_RED
    #undef AZSTD_RBTREE_BLACK
    #undef AZSTD_RBTREE_LEFT
    #undef AZSTD_RBTREE_RIGHT
}

#endif // AZSTD_INTRUSIVE_SET_H
#pragma once

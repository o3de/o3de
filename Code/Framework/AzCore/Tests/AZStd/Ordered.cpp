/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>

#include <AzCore/std/containers/intrusive_set.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/ranges/transform_view.h>

#define AZ_TEST_VALIDATE_EMPTY_TREE(_Tree_) \
    EXPECT_EQ(0, _Tree_.size());     \
    EXPECT_TRUE(_Tree_.empty());         \
    EXPECT_TRUE(_Tree_.begin() == _Tree_.end());

#define AZ_TEST_VALIDATE_TREE(_Tree_, _NumElements)                                                    \
    EXPECT_EQ(_NumElements, _Tree_.size());                                                     \
    EXPECT_TRUE((_Tree_.size() > 0) ? !_Tree_.empty() : _Tree_.empty());                              \
    EXPECT_TRUE((_NumElements > 0) ? _Tree_.begin() != _Tree_.end() : _Tree_.begin() == _Tree_.end()); \
    EXPECT_FALSE(_Tree_.empty());

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    /**
     * Setup the red-black tree. To achieve fixed_rbtree all you need is to use \ref AZStd::static_pool_allocator with
     * the node (Internal::rb_tree_node<T>).
     */
    template<class T, class KeyEq = AZStd::less<T>, class Allocator = AZStd::allocator >
    struct RedBlackTree_SetTestTraits
    {
        typedef T                   key_type;
        typedef KeyEq               key_equal;
        typedef T                   value_type;
        typedef Allocator           allocator_type;
        static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
    };

    class Tree_RedBlack
        : public LeakDetectionFixture
    {
    };

    TEST_F(Tree_RedBlack, Test)
    {
        array<int, 5> elements = {
            {1, 5, 8, 8, 4}
        };
        AZStd::size_t num;

        typedef rbtree<RedBlackTree_SetTestTraits<int> > rbtree_set_type;

        rbtree_set_type tree;
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        rbtree_set_type copy_tree = tree;
        AZ_TEST_VALIDATE_EMPTY_TREE(copy_tree);

        // insert unique element
        bool isInsert = tree.insert_unique(10).second;
        AZ_TEST_VALIDATE_TREE(tree, 1);
        AZ_TEST_ASSERT(isInsert == true);

        // insert unique element, which we already have.
        isInsert = tree.insert_unique(10).second;
        AZ_TEST_VALIDATE_TREE(tree, 1);
        AZ_TEST_ASSERT(isInsert == false);

        // insert equal element.
        tree.insert_equal(11);
        AZ_TEST_VALIDATE_TREE(tree, 2);

        // insert equal element, which we already have.
        tree.insert_equal(11);
        AZ_TEST_VALIDATE_TREE(tree, 3);

        tree.insert_unique(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 7);  /// there are 4 unique elements in the list.

        tree.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 12);

        AZ_TEST_ASSERT(*tree.begin() == 1);
        rbtree_set_type::iterator iterNext = tree.erase(tree.begin());  // we have 2 elements with value (1)
        AZ_TEST_ASSERT(*iterNext == 1);
        tree.erase(tree.begin());
        AZ_TEST_VALIDATE_TREE(tree, 10);
        AZ_TEST_ASSERT(*tree.begin() == 4);

        AZ_TEST_ASSERT(*prev(tree.end()) == 11);
        iterNext = tree.erase(prev(tree.end()));
        AZ_TEST_ASSERT(iterNext == tree.end());
        AZ_TEST_VALIDATE_TREE(tree, 9);
        AZ_TEST_ASSERT(*prev(tree.end()) == 11);  // we have 2 elements with value(11) at the end

        num = tree.erase(8);
        AZ_TEST_VALIDATE_TREE(tree, 6);
        AZ_TEST_ASSERT(num == 3);

        tree.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        tree.insert_equal(elements.begin(), elements.end());

        tree.erase(elements.begin(), elements.end());
        AZ_TEST_ASSERT(iterNext == tree.end());
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        tree.insert_equal(elements.begin(), elements.end());
        tree.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree, 10);

        rbtree_set_type::iterator iter, iter1;

        iter = tree.find(8);
        AZ_TEST_ASSERT(iter != tree.end());

        iter1 = tree.lower_bound(8);
        AZ_TEST_ASSERT(iter == iter1);

        iter1 = tree.upper_bound(8);
        AZ_TEST_ASSERT(iter1 == tree.end());
        AZ_TEST_ASSERT(iter1 != iter);

        num = tree.count(8);
        AZ_TEST_ASSERT(num == 4);

        AZStd::pair<rbtree_set_type::iterator, rbtree_set_type::iterator> range;
        range = tree.equal_range(8);
        AZ_TEST_ASSERT(range.first != tree.end());
        AZ_TEST_ASSERT(range.second == tree.end());
        AZ_TEST_ASSERT(AZStd::distance(range.first, range.second) == 4);

        // check the order
        int prevValue = *tree.begin();
        for (iter = next(tree.begin()); iter != tree.end(); ++iter)
        {
            AZ_TEST_ASSERT(prevValue <= *iter);
            prevValue = *iter;
        }

        // rbtree with different key equal function
        typedef rbtree<RedBlackTree_SetTestTraits<int, AZStd::greater<int> > > rbtree_set_type1;
        rbtree_set_type1 tree1;

        tree1.insert_equal(elements.begin(), elements.end());
        tree1.insert_equal(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_TREE(tree1, 10);

        // check the order
        prevValue = *tree1.begin();
        for (rbtree_set_type1::iterator it = next(tree1.begin()); it != tree1.end(); ++it)
        {
            AZ_TEST_ASSERT(prevValue >= *it);
            prevValue = *it;
        }
    }

    class Tree_IntrusiveMultiSet
        : public LeakDetectionFixture
    {
    public:
        struct Node
            : public AZStd::intrusive_multiset_node<Node>
        {
            Node(int order)
                : m_order(order) {}

            operator int() {
                return m_order;
            }

            int m_order;
        };

        typedef AZStd::intrusive_multiset<Node, AZStd::intrusive_multiset_base_hook<Node>, AZStd::less<> > IntrusiveSetIntKeyType;
    };

    TEST_F(Tree_IntrusiveMultiSet, Test)
    {
        IntrusiveSetIntKeyType  tree;

        // make sure the tree is empty
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);

        // insert node
        Node n1(1);
        tree.insert(&n1);
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 1);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        AZ_TEST_ASSERT(tree.begin()->m_order == 1);

        // insert node and sort
        Node n2(0);
        tree.insert(&n2);
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 2);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        AZ_TEST_ASSERT(tree.begin()->m_order == 0);
        AZ_TEST_ASSERT((++tree.begin())->m_order == 1);

        // erase the first node
        IntrusiveSetIntKeyType::iterator it = tree.erase(tree.begin());
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(tree.size() == 1);
        AZ_TEST_ASSERT(it->m_order == 1);

        // insert via reference (a pointer to it will be taken)
        it = tree.insert(n2);
        AZ_TEST_ASSERT(it->m_order == n2.m_order);
        AZ_TEST_ASSERT(tree.size() == 2);

        AZStd::fixed_vector<Node, 5> elements;
        elements.push_back(Node(2));
        elements.push_back(Node(5));
        elements.push_back(Node(8));
        elements.push_back(Node(8));
        elements.push_back(Node(4));

        // insert from input iterator
        tree.insert(elements.begin(), elements.end());
        AZ_TEST_ASSERT(!tree.empty());
        AZ_TEST_ASSERT(tree.size() == 7);
        AZ_TEST_ASSERT(tree.begin() != tree.end());
        it = tree.begin();
        AZ_TEST_ASSERT((it++)->m_order == 0);
        AZ_TEST_ASSERT((it++)->m_order == 1);
        AZ_TEST_ASSERT((it++)->m_order == 2);
        AZ_TEST_ASSERT((it++)->m_order == 4);
        AZ_TEST_ASSERT((it++)->m_order == 5);
        AZ_TEST_ASSERT((it++)->m_order == 8);
        AZ_TEST_ASSERT((it++)->m_order == 8);

        // lower bound
        it = tree.lower_bound(8);
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 8);
        AZ_TEST_ASSERT((++it)->m_order == 8);

        // upper bound
        it = tree.upper_bound(8);
        AZ_TEST_ASSERT(it == tree.end());
        AZ_TEST_ASSERT((--it)->m_order == 8);
        AZ_TEST_ASSERT((--it)->m_order == 8);

        // minimum
        it = tree.minimum();
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 0);

        // maximum
        it = tree.maximum();
        AZ_TEST_ASSERT(it != tree.end());
        AZ_TEST_ASSERT(it->m_order == 8);

        // erase elements with iterator
        tree.erase(elements.begin(), elements.end());
        it = tree.begin();
        AZ_TEST_ASSERT(tree.size() == 2);
        AZ_TEST_ASSERT((it++)->m_order == 0);
        AZ_TEST_ASSERT((it++)->m_order == 1);

        // clear the entire container
        tree.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(tree);
    }

    // SetContainerTest-Begin
    class Tree_Set
        : public LeakDetectionFixture
    {
    };

    TEST_F(Tree_Set, Test)
    {
        array<int, 5> elements = {
            {2, 6, 9, 3, 9}
        };
        array<int, 5> elements2 = {
            {11, 4, 13, 6, 1}
        };
        AZStd::size_t num;

        typedef set<int> int_set_type;

        int_set_type set;
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        int_set_type set1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(set1 > set);
        AZ_TEST_VALIDATE_TREE(set1, 4);
        AZ_TEST_ASSERT(*set1.begin() == 2);
        AZ_TEST_ASSERT(*prev(set1.end()) == 9);

        int_set_type set2(set1);
        AZ_TEST_ASSERT(set1 == set2);
        AZ_TEST_VALIDATE_TREE(set2, 4);
        AZ_TEST_ASSERT(*set2.begin() == 2);
        AZ_TEST_ASSERT(*prev(set2.end()) == 9);

        set = set2;
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prev(set.end()) == 9);

        set.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        set.swap(set2);
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prev(set.end()) == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(set2);

        bool isInsert = set.insert(6).second;
        AZ_TEST_VALIDATE_TREE(set, 4);
        AZ_TEST_ASSERT(isInsert == false);

        isInsert = set.insert(10).second;
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(isInsert == true);
        AZ_TEST_ASSERT(*prev(set.end()) == 10);

        set.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(set, 9);
        AZ_TEST_ASSERT(*set.begin() == 1);
        AZ_TEST_ASSERT(*prev(set.end()) == 13);

        set.erase(set.begin());
        AZ_TEST_VALIDATE_TREE(set, 8);
        AZ_TEST_ASSERT(*set.begin() == 2);

        set.erase(prev(set.end()));
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(*prev(set.end()) == 11);

        num = set.erase(6);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(num == 1);

        num = set.erase(100);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(num == 0);

        int_set_type::iterator iter = set.find(9);
        AZ_TEST_ASSERT(iter != set.end());

        iter = set.find(99);
        AZ_TEST_ASSERT(iter == set.end());

        num = set.count(9);
        AZ_TEST_ASSERT(num == 1);

        num = set.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = set.lower_bound(11);
        AZ_TEST_ASSERT(iter != set.end());
        AZ_TEST_ASSERT(*iter == 11);

        iter = set.lower_bound(111);
        AZ_TEST_ASSERT(iter == set.end());

        iter = set.upper_bound(11);
        AZ_TEST_ASSERT(iter == set.end()); // this is the last element

        iter = set.upper_bound(4);
        AZ_TEST_ASSERT(iter != set.end()); // this is NOT the last element

        iter = set.upper_bound(44);
        AZ_TEST_ASSERT(iter == set.end());

        AZStd::pair<int_set_type::iterator, int_set_type::iterator> range = set.equal_range(11);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 1);
        AZ_TEST_ASSERT(range.first != set.end());
        AZ_TEST_ASSERT(*range.first == 11);
        AZ_TEST_ASSERT(range.second == set.end()); // this is the last element

        range = set.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == set.end());
        AZ_TEST_ASSERT(range.second == set.end());
    }

    TEST_F(Tree_Set, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::set<int, AZStd::less<int>, AZ::AZStdIAllocator> setWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        auto insertIter = setWithCustomAllocator.emplace(1);
        EXPECT_TRUE(insertIter.second);
        insertIter = setWithCustomAllocator.emplace(1);
        EXPECT_FALSE(insertIter.second);
        EXPECT_EQ(1, setWithCustomAllocator.size());
    }

    class Tree_MultiSet
        : public LeakDetectionFixture
    {
    };

    TEST_F(Tree_MultiSet, Test)
    {
        array<int, 5> elements = {
            {2, 6, 9, 3, 9}
        };
        array<int, 5> elements2 = {
            {11, 4, 13, 6, 1}
        };
        AZStd::size_t num;

        typedef multiset<int> int_multiset_type;

        int_multiset_type set;
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        int_multiset_type set1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(set1 > set);
        AZ_TEST_VALIDATE_TREE(set1, 5);
        AZ_TEST_ASSERT(*set1.begin() == 2);
        AZ_TEST_ASSERT(*prev(set1.end()) == 9);

        int_multiset_type set2(set1);
        AZ_TEST_ASSERT(set1 == set2);
        AZ_TEST_VALIDATE_TREE(set2, 5);
        AZ_TEST_ASSERT(*set2.begin() == 2);
        AZ_TEST_ASSERT(*prev(set2.end()) == 9);

        set = set2;
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prev(set.end()) == 9);

        set.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(set);

        set.swap(set2);
        AZ_TEST_VALIDATE_TREE(set, 5);
        AZ_TEST_ASSERT(*set.begin() == 2);
        AZ_TEST_ASSERT(*prev(set.end()) == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(set2);

        set.insert(10);
        AZ_TEST_VALIDATE_TREE(set, 6);
        AZ_TEST_ASSERT(*prev(set.end()) == 10);

        set.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(set, 11);
        AZ_TEST_ASSERT(*set.begin() == 1);
        AZ_TEST_ASSERT(*prev(set.end()) == 13);

        set.erase(set.begin());
        AZ_TEST_VALIDATE_TREE(set, 10);
        AZ_TEST_ASSERT(*set.begin() == 2);

        set.erase(prev(set.end()));
        AZ_TEST_VALIDATE_TREE(set, 9);
        AZ_TEST_ASSERT(*prev(set.end()) == 11);

        num = set.erase(6);
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(num == 2);

        num = set.erase(100);
        AZ_TEST_VALIDATE_TREE(set, 7);
        AZ_TEST_ASSERT(num == 0);

        int_multiset_type::iterator iter = set.find(9);
        AZ_TEST_ASSERT(iter != set.end());

        iter = set.find(99);
        AZ_TEST_ASSERT(iter == set.end());

        num = set.count(9);
        AZ_TEST_ASSERT(num == 2);

        num = set.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = set.lower_bound(11);
        AZ_TEST_ASSERT(iter != set.end());
        AZ_TEST_ASSERT(*iter == 11);

        iter = set.lower_bound(111);
        AZ_TEST_ASSERT(iter == set.end());

        iter = set.upper_bound(11);
        AZ_TEST_ASSERT(iter == set.end()); // this is the last element

        iter = set.upper_bound(4);
        AZ_TEST_ASSERT(iter != set.end()); // this is NOT the last element

        iter = set.upper_bound(44);
        AZ_TEST_ASSERT(iter == set.end());

        AZStd::pair<int_multiset_type::iterator, int_multiset_type::iterator> range = set.equal_range(9);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 2);
        AZ_TEST_ASSERT(range.first != set.end());
        AZ_TEST_ASSERT(*range.first == 9);
        AZ_TEST_ASSERT(range.second != set.end());

        range = set.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == set.end());
        AZ_TEST_ASSERT(range.second == set.end());
    }

    TEST_F(Tree_MultiSet, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::multiset<int, AZStd::less<int>, AZ::AZStdIAllocator> setWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        setWithCustomAllocator.emplace(1);
        setWithCustomAllocator.emplace(1);
        EXPECT_EQ(2, setWithCustomAllocator.size());
    }

    class Tree_Map
        : public LeakDetectionFixture
    {
    };

    TEST_F(Tree_Map, Test)
    {
        fixed_vector<AZStd::pair<int, int>, 5> elements;
        elements.push_back(AZStd::make_pair(2, 100));
        elements.push_back(AZStd::make_pair(6, 101));
        elements.push_back(AZStd::make_pair(9, 102));
        elements.push_back(AZStd::make_pair(3, 103));
        elements.push_back(AZStd::make_pair(9, 104));
        fixed_vector<AZStd::pair<int, int>, 5> elements2;
        elements2.push_back(AZStd::make_pair(11, 200));
        elements2.push_back(AZStd::make_pair(4, 201));
        elements2.push_back(AZStd::make_pair(13, 202));
        elements2.push_back(AZStd::make_pair(6, 203));
        elements2.push_back(AZStd::make_pair(1, 204));
        AZStd::size_t num;

        typedef map<int, int> int_map_type;

        int_map_type map;
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        int_map_type map1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(map1 > map);
        AZ_TEST_VALIDATE_TREE(map1, 4);
        AZ_TEST_ASSERT((*map1.begin()).first == 2);
        AZ_TEST_ASSERT((*prev(map1.end())).first == 9);

        int_map_type map2(map1);
        AZ_TEST_ASSERT(map1 == map2);
        AZ_TEST_VALIDATE_TREE(map2, 4);
        AZ_TEST_ASSERT((*map2.begin()).first == 2);
        AZ_TEST_ASSERT((*prev(map2.end())).first == 9);

        map = map2;
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT((*map.begin()).first == 2);
        AZ_TEST_ASSERT((*prev(map.end())).first == 9);

        map.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        map.swap(map2);
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT((*map.begin()).first == 2);
        AZ_TEST_ASSERT((*prev(map.end())).first == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(map2);

        bool isInsert = map.insert(6).second;
        AZ_TEST_VALIDATE_TREE(map, 4);
        AZ_TEST_ASSERT(isInsert == false);

        isInsert = map.insert(10).second;
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(isInsert == true);
        AZ_TEST_ASSERT((*prev(map.end())).first == 10);

        map.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(map, 9);
        AZ_TEST_ASSERT((*map.begin()).first == 1);
        AZ_TEST_ASSERT((*prev(map.end())).first == 13);

        map.erase(map.begin());
        AZ_TEST_VALIDATE_TREE(map, 8);
        AZ_TEST_ASSERT(map.begin()->first == 2);

        map.erase(prev(map.end()));
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(prev(map.end())->first == 11);

        num = map.erase(6);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(num == 1);

        num = map.erase(100);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(num == 0);

        int_map_type::iterator iter = map.find(9);
        AZ_TEST_ASSERT(iter != map.end());

        iter = map.find(99);
        AZ_TEST_ASSERT(iter == map.end());

        num = map.count(9);
        AZ_TEST_ASSERT(num == 1);

        num = map.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = map.lower_bound(11);
        AZ_TEST_ASSERT(iter != map.end());
        AZ_TEST_ASSERT(iter->first == 11);

        iter = map.lower_bound(111);
        AZ_TEST_ASSERT(iter == map.end());

        iter = map.upper_bound(11);
        AZ_TEST_ASSERT(iter == map.end()); // this is the last element

        iter = map.upper_bound(4);
        AZ_TEST_ASSERT(iter != map.end()); // this is NOT the last element

        iter = map.upper_bound(44);
        AZ_TEST_ASSERT(iter == map.end());

        AZStd::pair<int_map_type::iterator, int_map_type::iterator> range = map.equal_range(11);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 1);
        AZ_TEST_ASSERT(range.first != map.end());
        AZ_TEST_ASSERT(range.first->first == 11);
        AZ_TEST_ASSERT(range.second == map.end()); // this is the last element

        range = map.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == map.end());
        AZ_TEST_ASSERT(range.second == map.end());

        AZStd::map<int, MyNoCopyClass> nocopy_map;
        MyNoCopyClass& inserted = nocopy_map.insert(3).first->second;
        inserted.m_bool = true;
        AZ_TEST_ASSERT(nocopy_map.begin()->second.m_bool == true);

        int_map_type map3({
                {1, 2}, {3, 4}, {5, 6}
            });
        AZ_TEST_VALIDATE_TREE(map3, 3);
        AZ_TEST_ASSERT((*map3.begin()).first == 1);
        AZ_TEST_ASSERT((*map3.begin()).second == 2);
        AZ_TEST_ASSERT((*prev(map3.end())).first == 5);
        AZ_TEST_ASSERT((*prev(map3.end())).second == 6);
    }

    TEST_F(Tree_Map, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::map<int, int, AZStd::less<int>, AZ::AZStdIAllocator> mapWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        auto insertIter = mapWithCustomAllocator.emplace(1, 1);
        EXPECT_TRUE(insertIter.second);
        insertIter = mapWithCustomAllocator.emplace(1, 2);
        EXPECT_FALSE(insertIter.second);
        EXPECT_EQ(1, mapWithCustomAllocator.size());
    }

    TEST_F(Tree_Map, IndexOperatorCompilesWithMoveOnlyType)
    {
        AZStd::map<int, AZStd::unique_ptr<int>> uniquePtrIntMap;
        uniquePtrIntMap[4] = AZStd::make_unique<int>(74);
        auto findIter = uniquePtrIntMap.find(4);
        EXPECT_NE(uniquePtrIntMap.end(), findIter);
        EXPECT_EQ(74, *findIter->second);
    }

    class Tree_MultiMap
        : public LeakDetectionFixture
    {
    };

    TEST_F(Tree_MultiMap, Test)
    {
        fixed_vector<AZStd::pair<int, int>, 5> elements;
        elements.push_back(AZStd::make_pair(2, 100));
        elements.push_back(AZStd::make_pair(6, 101));
        elements.push_back(AZStd::make_pair(9, 102));
        elements.push_back(AZStd::make_pair(3, 103));
        elements.push_back(AZStd::make_pair(9, 104));
        fixed_vector<AZStd::pair<int, int>, 5> elements2;
        elements2.push_back(AZStd::make_pair(11, 200));
        elements2.push_back(AZStd::make_pair(4, 201));
        elements2.push_back(AZStd::make_pair(13, 202));
        elements2.push_back(AZStd::make_pair(6, 203));
        elements2.push_back(AZStd::make_pair(1, 204));
        AZStd::size_t num;

        typedef multimap<int, int> int_multimap_type;

        int_multimap_type map;
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        int_multimap_type map1(elements.begin(), elements.end());
        AZ_TEST_ASSERT(map1 > map);
        AZ_TEST_VALIDATE_TREE(map1, 5);
        AZ_TEST_ASSERT(map1.begin()->first == 2);
        AZ_TEST_ASSERT(prev(map1.end())->first == 9);

        int_multimap_type map2(map1);
        AZ_TEST_ASSERT(map1 == map2);
        AZ_TEST_VALIDATE_TREE(map2, 5);
        AZ_TEST_ASSERT(map2.begin()->first == 2);
        AZ_TEST_ASSERT(prev(map2.end())->first == 9);

        map = map2;
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(map.begin()->first == 2);
        AZ_TEST_ASSERT(prev(map.end())->first == 9);

        map.clear();
        AZ_TEST_VALIDATE_EMPTY_TREE(map);

        map.swap(map2);
        AZ_TEST_VALIDATE_TREE(map, 5);
        AZ_TEST_ASSERT(map.begin()->first == 2);
        AZ_TEST_ASSERT(prev(map.end())->first == 9);
        AZ_TEST_VALIDATE_EMPTY_TREE(map2);

        map.insert(10);
        AZ_TEST_VALIDATE_TREE(map, 6);
        AZ_TEST_ASSERT(prev(map.end())->first == 10);

        map.insert(elements2.begin(), elements2.end());
        AZ_TEST_VALIDATE_TREE(map, 11);
        AZ_TEST_ASSERT(map.begin()->first == 1);
        AZ_TEST_ASSERT(prev(map.end())->first == 13);

        map.erase(map.begin());
        AZ_TEST_VALIDATE_TREE(map, 10);
        AZ_TEST_ASSERT(map.begin()->first == 2);

        map.erase(prev(map.end()));
        AZ_TEST_VALIDATE_TREE(map, 9);
        AZ_TEST_ASSERT(prev(map.end())->first == 11);

        num = map.erase(6);
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(num == 2);

        num = map.erase(100);
        AZ_TEST_VALIDATE_TREE(map, 7);
        AZ_TEST_ASSERT(num == 0);

        int_multimap_type::iterator iter = map.find(9);
        AZ_TEST_ASSERT(iter != map.end());

        iter = map.find(99);
        AZ_TEST_ASSERT(iter == map.end());

        num = map.count(9);
        AZ_TEST_ASSERT(num == 2);

        num = map.count(88);
        AZ_TEST_ASSERT(num == 0);

        iter = map.lower_bound(11);
        AZ_TEST_ASSERT(iter != map.end());
        AZ_TEST_ASSERT(iter->first == 11);

        iter = map.lower_bound(111);
        AZ_TEST_ASSERT(iter == map.end());

        iter = map.upper_bound(11);
        AZ_TEST_ASSERT(iter == map.end()); // this is the last element

        iter = map.upper_bound(4);
        AZ_TEST_ASSERT(iter != map.end()); // this is NOT the last element

        iter = map.upper_bound(44);
        AZ_TEST_ASSERT(iter == map.end());

        AZStd::pair<int_multimap_type::iterator, int_multimap_type::iterator> range = map.equal_range(9);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 2);
        AZ_TEST_ASSERT(range.first != map.end());
        AZ_TEST_ASSERT(range.first->first == 9);
        AZ_TEST_ASSERT(range.second != map.end());

        range = map.equal_range(91);
        num = distance(range.first, range.second);
        AZ_TEST_ASSERT(num == 0);
        AZ_TEST_ASSERT(range.first == map.end());
        AZ_TEST_ASSERT(range.second == map.end());

        int_multimap_type intint_map3({
                {1, 10}, {2, 200}, {3, 3000}, {4, 40000}, {4, 40001}, {5, 500000}
            });
        AZ_TEST_ASSERT(intint_map3.size() == 6);
        AZ_TEST_ASSERT(intint_map3.count(1) == 1);
        AZ_TEST_ASSERT(intint_map3.count(2) == 1);
        AZ_TEST_ASSERT(intint_map3.count(3) == 1);
        AZ_TEST_ASSERT(intint_map3.count(4) == 2);
        AZ_TEST_ASSERT(intint_map3.count(5) == 1);
        AZ_TEST_ASSERT(intint_map3.lower_bound(1)->second == 10);
        AZ_TEST_ASSERT(intint_map3.lower_bound(2)->second == 200);
        AZ_TEST_ASSERT(intint_map3.lower_bound(3)->second == 3000);
        AZ_TEST_ASSERT(intint_map3.lower_bound(4)->second == 40000 || intint_map3.lower_bound(4)->second == 40001);
        AZ_TEST_ASSERT((++intint_map3.lower_bound(4))->second == 40000 || (++intint_map3.lower_bound(4))->second == 40001);
        AZ_TEST_ASSERT(intint_map3.lower_bound(5)->second == 500000);
    }

    TEST_F(Tree_MultiMap, ExplicitAllocatorSucceeds)
    {
        AZ::OSAllocator customAllocator;
        AZStd::multimap<int, int, AZStd::less<int>, AZ::AZStdIAllocator> mapWithCustomAllocator{ AZ::AZStdIAllocator(&customAllocator) };
        mapWithCustomAllocator.emplace(1, 1);
        mapWithCustomAllocator.emplace(1, 2);
        EXPECT_EQ(2, mapWithCustomAllocator.size());
    }

    template<typename ContainerType>
    class TreeSetContainers
        : public LeakDetectionFixture
    {
    };


    struct MoveOnlyIntType
    {
        MoveOnlyIntType() = default;
        MoveOnlyIntType(int32_t value)
            : m_value(value)
        {}
        MoveOnlyIntType(const MoveOnlyIntType&) = delete;
        MoveOnlyIntType& operator=(const MoveOnlyIntType&) = delete;
        MoveOnlyIntType(MoveOnlyIntType&& other)
            : m_value(other.m_value)
        {
        }

        MoveOnlyIntType& operator=(MoveOnlyIntType&& other)
        {
            m_value = other.m_value;
            other.m_value = {};
            return *this;
        }

        explicit operator int32_t()
        {
            return m_value;
        }

        bool operator==(const MoveOnlyIntType& other) const
        {
            return m_value == other.m_value;
        }

        int32_t m_value;
    };

    struct MoveOnlyIntTypeCompare
    {
        bool operator()(const MoveOnlyIntType& lhs, const MoveOnlyIntType& rhs) const
        {
            return lhs.m_value < rhs.m_value;
        }
    };

    template<typename ContainerType>
    struct TreeSetConfig
    {
        using SetType = ContainerType;
        static SetType Create()
        {
            SetType testSet;

            testSet.emplace(1);
            testSet.emplace(2);
            testSet.emplace(84075);
            testSet.emplace(-73);
            testSet.emplace(534);
            testSet.emplace(-845920);
            testSet.emplace(-42);
            testSet.emplace(0b1111'0000);
            return testSet;
        }
    };

    using SetContainerConfigs = ::testing::Types<
        TreeSetConfig<AZStd::set<int32_t>>
        , TreeSetConfig<AZStd::multiset<int32_t>>
        , TreeSetConfig<AZStd::set<MoveOnlyIntType, MoveOnlyIntTypeCompare>>
        , TreeSetConfig<AZStd::multiset<MoveOnlyIntType, MoveOnlyIntTypeCompare>>
    >;
    TYPED_TEST_CASE(TreeSetContainers, SetContainerConfigs);

    TYPED_TEST(TreeSetContainers, ExtractNodeHandleByKeySucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(84075);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(84075, static_cast<int32_t>(extractedNode.value()));
    }

    TYPED_TEST(TreeSetContainers, ExtractNodeHandleByKeyFails)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(10000001);

        EXPECT_EQ(8, testContainer.size());
        EXPECT_TRUE(extractedNode.empty());
    }

    TYPED_TEST(TreeSetContainers, ExtractNodeHandleByIteratorSucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(-73);
        node_type extractedNode = testContainer.extract(foundIter);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(-73, static_cast<int32_t>(extractedNode.value()));
    }

    TYPED_TEST(TreeSetContainers, ExtractAndReinsertNodeHandleByIteratorSucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(-73);
        auto nextIter = AZStd::next(foundIter);
        node_type extractedNode = testContainer.extract(foundIter);
        extractedNode.value() = static_cast<int32_t>(extractedNode.value()) + 1;
        testContainer.insert(nextIter, AZStd::move(extractedNode));

        // Lookup reinserted node
        foundIter = testContainer.find(-72);
        ASSERT_NE(testContainer.end(), foundIter);
    }

    TYPED_TEST(TreeSetContainers, InsertNodeHandleSucceeds)
    {
        using SetType = AZStd::set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = TreeSetConfig<SetType>::Create();
        node_type extractedNode = testContainer.extract(84075);
        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(84075, static_cast<int32_t>(extractedNode.value()));

        extractedNode.value() = -60;
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());

        EXPECT_NE(0, testContainer.count(-60));
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Set, SetInsertNodeHandleSucceeds)
    {
        using SetType = AZStd::set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = TreeSetConfig<SetType>::Create();

        node_type extractedNode;
        EXPECT_TRUE(extractedNode.empty());

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_EQ(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Set, SetInsertNodeHandleWithEmptyNodeHandleFails)
    {
        using SetType = AZStd::set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = TreeSetConfig<SetType>::Create();

        node_type extractedNode;
        EXPECT_TRUE(extractedNode.empty());

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_EQ(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Set, SetInsertNodeHandleWithDuplicateValueInNodeHandleFails)
    {
        using SetType = AZStd::set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = TreeSetConfig<SetType>::Create();
        node_type extractedNode = testContainer.extract(2);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(2, static_cast<int32_t>(extractedNode.value()));

        // Set node handle value to a key that is currently within the container
        extractedNode.value() = 534;
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_FALSE(insertResult.node.empty());

        EXPECT_EQ(7, testContainer.size());
    }

    TEST_F(Tree_Set, SetExtractedNodeCanBeInsertedIntoMultiset)
    {
        using SetType = AZStd::set<int32_t>;
        using MultisetType = AZStd::multiset<int32_t>;

        SetType uniqueSet{ 1, 2, 3 };
        MultisetType multiSet{ 1, 4, 1 };

        auto extractedNode = uniqueSet.extract(1);
        EXPECT_EQ(2, uniqueSet.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertIt = multiSet.insert(AZStd::move(extractedNode));
        EXPECT_NE(multiSet.end(), insertIt);
        EXPECT_EQ(4, multiSet.size());
        EXPECT_EQ(3, multiSet.count(1));
    }

    TEST_F(Tree_Set, MultisetExtractedNodeCanBeInsertedIntoSet)
    {
        using SetType = AZStd::set<int32_t>;
        using MultisetType = AZStd::multiset<int32_t>;

        SetType uniqueSet{ 1, 2, 3 };
        MultisetType multiSet{ 1, 4, 1 };

        auto extractedNode = multiSet.extract(4);
        EXPECT_EQ(2, multiSet.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertResult = uniqueSet.insert(AZStd::move(extractedNode));
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_EQ(4, uniqueSet.size());
        EXPECT_EQ(1, uniqueSet.count(4));
    }

    template<template<class...> class SetTemplate>
    static void TreeSetRangeConstructorSucceeds()
    {
        constexpr AZStd::string_view testView = "abc";

        SetTemplate testSet(AZStd::from_range, testView);
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));

        testSet = SetTemplate(AZStd::from_range, AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::list<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::deque<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::unordered_set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::fixed_vector<char, 8>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::array{ 'a', 'b', 'c' });
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));

        AZStd::fixed_string<8> testValue(testView);
        testSet = SetTemplate(AZStd::from_range, testValue);
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::string(testView));
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c'));

        // Test Range views
        testSet = SetTemplate(AZStd::from_range, testValue | AZStd::views::transform([](const char elem) -> char { return elem + 1; }));
        EXPECT_THAT(testSet, ::testing::ElementsAre('b', 'c', 'd'));
    }

    TEST_F(Tree_Set, RangeConstructor_Succeeds)
    {
        TreeSetRangeConstructorSucceeds<AZStd::set>();
        TreeSetRangeConstructorSucceeds<AZStd::multiset>();
    }

    template<template<class...> class SetTemplate>
    static void TreeSetInsertRangeSucceeds()
    {
        constexpr AZStd::string_view testView = "abc";
        SetTemplate testSet{ 'd', 'e', 'f' };
        testSet.insert_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testSet.insert_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + 6; }));
        EXPECT_THAT(testSet, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(Tree_Set, InsertRange_Succeeds)
    {
        TreeSetInsertRangeSucceeds<AZStd::set>();
        TreeSetInsertRangeSucceeds<AZStd::multiset>();
    }

    template <typename ContainerType>
    class TreeSetDifferentAllocatorFixture
        : public LeakDetectionFixture
    {
    };

    template<template <typename, typename, typename> class ContainerTemplate>
    struct TreeSetWithCustomAllocatorConfig
    {
        using ContainerType = ContainerTemplate<int32_t, AZStd::less<int32_t>, AZ::AZStdIAllocator>;

        static ContainerType Create(std::initializer_list<typename ContainerType::value_type> intList, AZ::IAllocator* allocatorInstance)
        {
            ContainerType allocatorSet(intList, AZStd::less<int32_t>{}, AZ::AZStdIAllocator{ allocatorInstance });
            return allocatorSet;
        }
    };

    using SetTemplateConfigs = ::testing::Types<
        TreeSetWithCustomAllocatorConfig<AZStd::set>
        , TreeSetWithCustomAllocatorConfig<AZStd::multiset>
    >;
    TYPED_TEST_CASE(TreeSetDifferentAllocatorFixture, SetTemplateConfigs);

#if GTEST_HAS_DEATH_TEST
#if AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    TYPED_TEST(TreeSetDifferentAllocatorFixture, DISABLED_InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#else
    TYPED_TEST(TreeSetDifferentAllocatorFixture, InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#endif // AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorSet = TypeParam::Create({ {1}, {2}, {3} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        auto extractedNode = systemAllocatorSet.extract(1);

        ContainerType osAllocatorSet = TypeParam::Create({ {2} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Attempt to insert an extracted node from a container using the AZ::SystemAllocator into a container using the AZ::OSAllocator
        EXPECT_DEATH(
            {
                UnitTest::TestRunner::Instance().StartAssertTests();
                osAllocatorSet.insert(AZStd::move(extractedNode));
                if (UnitTest::TestRunner::Instance().StopAssertTests() > 0)
                {
                    // AZ_Assert does not cause the application to exit in profile_test configuration
                    // Therefore an exit with a non-zero error code is invoked to trigger the death condition
                    exit(1);
                }
            }, ".*");
    }
#endif // GTEST_HAS_DEATH_TEST

    TYPED_TEST(TreeSetDifferentAllocatorFixture, SwapMovesElementsWhenAllocatorsDiffer)
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorMap = TypeParam::Create({ {1}, {2}, {3} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        ContainerType osAllocatorMap = TypeParam::Create({ {2} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Swap the container elements around while leave the allocators the same.
        systemAllocatorMap.swap(osAllocatorMap);

        EXPECT_EQ(1, systemAllocatorMap.size());
        EXPECT_EQ(1, systemAllocatorMap.count(2));
        EXPECT_EQ(AZ::AZStdIAllocator(&AZ::AllocatorInstance<AZ::SystemAllocator>::Get()), systemAllocatorMap.get_allocator());

        EXPECT_EQ(3, osAllocatorMap.size());
        EXPECT_EQ(1, osAllocatorMap.count(1));
        EXPECT_EQ(1, osAllocatorMap.count(2));
        EXPECT_EQ(1, osAllocatorMap.count(3));
        EXPECT_EQ(AZ::AZStdIAllocator(&AZ::AllocatorInstance<AZ::OSAllocator>::Get()), osAllocatorMap.get_allocator());
    }

    template<typename ContainerType>
    class TreeMapContainers
        : public LeakDetectionFixture
    {
    };

    template<typename ContainerType>
    struct TreeMapConfig
    {
        using MapType = ContainerType;
        static MapType Create()
        {
            MapType testMap;

            testMap.emplace(8001, 1337);
            testMap.emplace(-200, 31337);
            testMap.emplace(-932, 0xbaddf00d);
            testMap.emplace(73, 0xfee1badd);
            testMap.emplace(1872, 0xCDCDCDCD);
            testMap.emplace(0xFF, 7000000);
            testMap.emplace(0777, 0b00110000010);
            testMap.emplace(0b11010110110000101, 0xDDDDDDDD);
            return testMap;
        }
    };
    using MapContainerConfigs = ::testing::Types<
        TreeMapConfig<AZStd::map<int32_t, int32_t>>
        , TreeMapConfig<AZStd::multimap<int32_t, int32_t>>
        , TreeMapConfig<AZStd::map<MoveOnlyIntType, int32_t, MoveOnlyIntTypeCompare>>
        , TreeMapConfig<AZStd::multimap<MoveOnlyIntType, int32_t, MoveOnlyIntTypeCompare>>
    >;
    TYPED_TEST_CASE(TreeMapContainers, MapContainerConfigs);

    TYPED_TEST(TreeMapContainers, ExtractNodeHandleByKeySucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(0777);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0777, static_cast<int32_t>(extractedNode.key()));
        EXPECT_EQ(0b00110000010, extractedNode.mapped());
    }

    TYPED_TEST(TreeMapContainers, ExtractNodeHandleByKeyFails)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(3);

        EXPECT_EQ(8, testContainer.size());
        EXPECT_TRUE(extractedNode.empty());
    }

    TYPED_TEST(TreeMapContainers, ExtractNodeHandleByIteratorSucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(73);
        node_type extractedNode = testContainer.extract(foundIter);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(73, static_cast<int32_t>(extractedNode.key()));
        EXPECT_EQ(0xfee1badd, extractedNode.mapped());
    }

    TYPED_TEST(TreeMapContainers, ExtractAndReinsertNodeHandleByIteratorSucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(73);
        auto nextIter = AZStd::next(foundIter);
        node_type extractedNode = testContainer.extract(foundIter);
        extractedNode.key() = static_cast<int32_t>(extractedNode.key()) + 1;
        testContainer.insert(nextIter, AZStd::move(extractedNode));

        foundIter = testContainer.find(74);
        ASSERT_NE(testContainer.end(), foundIter);
    }

    TYPED_TEST(TreeMapContainers, InsertNodeHandleSucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(0777);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0777, static_cast<int32_t>(extractedNode.key()));
        extractedNode.key() = 0644;
        extractedNode.mapped() = 1'212;

        testContainer.insert(AZStd::move(extractedNode));
        auto foundIt = testContainer.find(0644);
        EXPECT_NE(testContainer.end(), foundIt);
        EXPECT_EQ(0644, static_cast<int32_t>(foundIt->first));
        EXPECT_EQ(1'212, foundIt->second);
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Map, MapInsertNodeHandleSucceeds)
    {
        using MapType = AZStd::map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;

        MapType testContainer = TreeMapConfig<MapType>::Create();
        node_type extractedNode = testContainer.extract(0b11010110110000101);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0b11010110110000101, extractedNode.key());

        extractedNode.key() = -60;
        extractedNode.mapped() = -1;

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());

        auto foundIt = testContainer.find(-60);
        EXPECT_NE(testContainer.end(), foundIt);
        EXPECT_EQ(-60, foundIt->first);
        EXPECT_EQ(-1, foundIt->second);
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Map, MapInsertNodeHandleWithEmptyNodeHandleFails)
    {
        using MapType = AZStd::map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;

        MapType testContainer = TreeMapConfig<MapType>::Create();

        node_type extractedNode;
        EXPECT_TRUE(extractedNode.empty());

        EXPECT_EQ(8, testContainer.size());
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_EQ(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(Tree_Map, MapInsertNodeHandleWithDuplicateValueInNodeHandleFails)
    {
        using MapType = AZStd::map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;
        MapType testContainer = TreeMapConfig<MapType>::Create();

        node_type extractedNode = testContainer.extract(0xFF);
        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0xFF, static_cast<int32_t>(extractedNode.key()));
        // Update the extracted node to have the same key as one of the elements currently within the container
        extractedNode.key() = -200;

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_FALSE(insertResult.node.empty());
        EXPECT_EQ(7, testContainer.size());
    }

    TEST_F(Tree_Map, MapExtractedNodeCanBeInsertedIntoMultimap)
    {
        using MapType = AZStd::map<int32_t, int32_t>;
        using MultimapType = AZStd::multimap<int32_t, int32_t>;

        MapType uniqueMap{ {1, 2}, {2, 4}, {3, 6} };
        MultimapType multiMap{ {1, 2}, { 4, 8}, {1, 3} };

        auto extractedNode = uniqueMap.extract(1);
        EXPECT_EQ(2, uniqueMap.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertIt = multiMap.insert(AZStd::move(extractedNode));
        EXPECT_NE(multiMap.end(), insertIt);
        EXPECT_EQ(4, multiMap.size());
        EXPECT_EQ(3, multiMap.count(1));
    }

    TEST_F(Tree_Map, MultimapExtractedNodeCanBeInsertedIntoMap)
    {
        using MapType = AZStd::map<int32_t, int32_t>;
        using MultimapType = AZStd::multimap<int32_t, int32_t>;

        MapType uniqueMap{ {1, 2}, {2, 4}, {3, 6} };
        MultimapType multiMap{ {1, 2}, { 4, 8}, {1, 3} };

        auto extractedNode = multiMap.extract(4);
        EXPECT_EQ(2, multiMap.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertResult = uniqueMap.insert(AZStd::move(extractedNode));
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_EQ(4, uniqueMap.size());
        EXPECT_EQ(1, uniqueMap.count(4));
    }

    TEST_F(Tree_Map, MapTryEmplace_DoesNotConstruct_OnExistingKey)
    {
        static int s_tryEmplaceConstructorCallCount;
        s_tryEmplaceConstructorCallCount = 0;
        struct TryEmplaceConstructorCalls
        {
            TryEmplaceConstructorCalls()
            {
                ++s_tryEmplaceConstructorCallCount;
            }
            TryEmplaceConstructorCalls(int value)
                : m_value{ value }
            {
                ++s_tryEmplaceConstructorCallCount;
            }
            TryEmplaceConstructorCalls(const TryEmplaceConstructorCalls& other)
                : m_value{ other.m_value }
            {
                ++s_tryEmplaceConstructorCallCount;
            }

            int m_value{};
        };
        using TryEmplaceTestMap = AZStd::map<int, TryEmplaceConstructorCalls>;
        TryEmplaceTestMap testContainer;

        // try_emplace move key
        AZStd::pair<TryEmplaceTestMap::iterator, bool> emplacePairIter = testContainer.try_emplace(1, 5);
        EXPECT_EQ(1, s_tryEmplaceConstructorCallCount);
        EXPECT_TRUE(emplacePairIter.second);
        EXPECT_EQ(5, emplacePairIter.first->second.m_value);

        // try_emplace copy key
        int testKey = 3;
        emplacePairIter = testContainer.try_emplace(testKey, 72);
        EXPECT_EQ(2, s_tryEmplaceConstructorCallCount);
        EXPECT_TRUE(emplacePairIter.second);
        EXPECT_EQ(72, emplacePairIter.first->second.m_value);

        // invoke try_emplace with hint and move key
        TryEmplaceTestMap::iterator emplaceIter = testContainer.try_emplace(testContainer.end(), 5, 4092);
        EXPECT_EQ(3, s_tryEmplaceConstructorCallCount);
        EXPECT_EQ(4092, emplaceIter->second.m_value);

        // invoke try_emplace with hint and copy key
        testKey = 48;
        emplaceIter = testContainer.try_emplace(testContainer.end(), testKey, 824);
        EXPECT_EQ(4, s_tryEmplaceConstructorCallCount);
        EXPECT_EQ(824, emplaceIter->second.m_value);

        // Since the key of '1' exist, nothing should be constructed
        emplacePairIter = testContainer.try_emplace(1, -6354);
        EXPECT_EQ(4, s_tryEmplaceConstructorCallCount);
        EXPECT_FALSE(emplacePairIter.second);
        EXPECT_EQ(5, emplacePairIter.first->second.m_value);
    }

    TEST_F(Tree_Map, MapTryEmplace_DoesNotMoveValue_OnExistingKey)
    {
        AZStd::unordered_map<int, AZStd::unique_ptr<int>> testMap;
        auto testPtr = AZStd::make_unique<int>(5);
        auto [emplaceIter, inserted] = testMap.try_emplace(1, AZStd::move(testPtr));
        EXPECT_TRUE(inserted);
        EXPECT_EQ(nullptr, testPtr);

        testPtr = AZStd::make_unique<int>(7000);
        auto [emplaceIter2, inserted2] = testMap.try_emplace(1, AZStd::move(testPtr));
        EXPECT_FALSE(inserted2);
        ASSERT_NE(nullptr, testPtr);
        EXPECT_EQ(7000, *testPtr);
    }

    TEST_F(Tree_Map, MapInsertOrAssign_PerformsAssignment_OnExistingKey)
    {
        static int s_tryInsertOrAssignConstructorCalls;
        static int s_tryInsertOrAssignAssignmentCalls;
        s_tryInsertOrAssignConstructorCalls = 0;
        s_tryInsertOrAssignAssignmentCalls = 0;
        struct InsertOrAssignInitCalls
        {
            InsertOrAssignInitCalls()
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }
            InsertOrAssignInitCalls(int value)
                : m_value{ value }
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }
            InsertOrAssignInitCalls(const InsertOrAssignInitCalls& other)
                : m_value{ other.m_value }
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }

            InsertOrAssignInitCalls& operator=(int value)
            {
                m_value = value;
                ++s_tryInsertOrAssignAssignmentCalls;
                return *this;
            }

            int m_value{};
        };
        using InsertOrAssignTestMap = AZStd::map<int, InsertOrAssignInitCalls>;
        InsertOrAssignTestMap testContainer;

        // insert_or_assign move key
        AZStd::pair<InsertOrAssignTestMap::iterator, bool> insertOrAssignPairIter = testContainer.insert_or_assign(1, 5);
        EXPECT_EQ(1, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_TRUE(insertOrAssignPairIter.second);
        EXPECT_EQ(5, insertOrAssignPairIter.first->second.m_value);

        // insert_or_assign copy key
        int testKey = 3;
        insertOrAssignPairIter = testContainer.insert_or_assign(testKey, 72);
        EXPECT_EQ(2, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_TRUE(insertOrAssignPairIter.second);
        EXPECT_EQ(72, insertOrAssignPairIter.first->second.m_value);

        // invoke insert_or_assign with hint and move key
        InsertOrAssignTestMap::iterator insertOrAssignIter = testContainer.insert_or_assign(testContainer.end(), 5, 4092);
        EXPECT_EQ(3, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_EQ(4092, insertOrAssignIter->second.m_value);

        // invoke insert_or_assign with hint and copy key
        testKey = 48;
        insertOrAssignIter = testContainer.insert_or_assign(testContainer.end(), testKey, 824);
        EXPECT_EQ(4, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_EQ(824, insertOrAssignIter->second.m_value);

        // Since the key of '1' exist, only an assignment should take place
        insertOrAssignPairIter = testContainer.insert_or_assign(1, -6354);
        EXPECT_EQ(4, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(1, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_FALSE(insertOrAssignPairIter.second);
        EXPECT_EQ(-6354, insertOrAssignPairIter.first->second.m_value);
    }

    template<template<class...> class MapTemplate>
    static void TreeMapRangeConstructorSucceeds()
    {
        using ValueType = AZStd::pair<char, int>;
        constexpr AZStd::array testArray{ ValueType{'a', 1}, ValueType{'b', 2}, ValueType{'c', 3} };

        MapTemplate testMap(AZStd::from_range, testArray);
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));

        testMap = MapTemplate(AZStd::from_range, AZStd::vector<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::list<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::deque<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::set<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::unordered_set<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::fixed_vector<ValueType, 8>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::span(testArray));
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
    }

    TEST_F(Tree_Map, RangeConstructor_Succeeds)
    {
        TreeMapRangeConstructorSucceeds<AZStd::map>();
        TreeMapRangeConstructorSucceeds<AZStd::multimap>();
    }

    template<template<class...> class MapTemplate>
    static void TreeMapInsertRangeSucceeds()
    {
        using ValueType = AZStd::pair<char, int>;
        constexpr AZStd::array testArray{ ValueType{'a', 1}, ValueType{'b', 2}, ValueType{'c', 3} };

        MapTemplate testMap(AZStd::from_range, testArray);

        testMap.insert_range(AZStd::vector<ValueType>{ValueType{ 'd', 4 }, ValueType{ 'e', 5 }, ValueType{ 'f', 6 }});
        EXPECT_THAT(testMap, ::testing::ElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 },
            ValueType{ 'd', 4 }, ValueType{ 'e', 5 }, ValueType{ 'f', 6 }));
    }

    TEST_F(Tree_Map, InsertRange_Succeeds)
    {
        TreeMapInsertRangeSucceeds<AZStd::map>();
        TreeMapInsertRangeSucceeds<AZStd::multimap>();
    }

    template <typename ContainerType>
    class TreeMapDifferentAllocatorFixture
        : public LeakDetectionFixture
    {
    };

    template<template <typename, typename, typename, typename> class ContainerTemplate>
    struct TreeMapWithCustomAllocatorConfig
    {
        using ContainerType = ContainerTemplate<int32_t, int32_t, AZStd::less<int32_t>, AZ::AZStdIAllocator>;

        static ContainerType Create(std::initializer_list<typename ContainerType::value_type> intList, AZ::IAllocator* allocatorInstance)
        {
            ContainerType allocatorMap(intList, AZStd::less<int32_t>{}, AZ::AZStdIAllocator{ allocatorInstance });
            return allocatorMap;
        }
    };

    using MapTemplateConfigs = ::testing::Types<
        TreeMapWithCustomAllocatorConfig<AZStd::map>
        , TreeMapWithCustomAllocatorConfig<AZStd::multimap>
    >;
    TYPED_TEST_CASE(TreeMapDifferentAllocatorFixture, MapTemplateConfigs);

#if GTEST_HAS_DEATH_TEST
#if AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    TYPED_TEST(TreeMapDifferentAllocatorFixture, DISABLED_InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#else
    TYPED_TEST(TreeMapDifferentAllocatorFixture, InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#endif // AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorMap = TypeParam::Create({ {1, 2}, {2, 4}, {3, 6} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        auto extractedNode = systemAllocatorMap.extract(1);

        ContainerType osAllocatorMap = TypeParam::Create({ {2, 4} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Attempt to insert an extracted node from a container using the AZ::SystemAllocator into a container using the AZ::OSAllocator
        EXPECT_DEATH(
            {
                UnitTest::TestRunner::Instance().StartAssertTests();
                osAllocatorMap.insert(AZStd::move(extractedNode));
                if (UnitTest::TestRunner::Instance().StopAssertTests() > 0)
                {
                    // AZ_Assert does not cause the application to exit in profile_test configuration
                    // Therefore an exit with a non-zero error code is invoked to trigger the death condition
                    exit(1);
                }
            }, ".*");
    }
#endif // GTEST_HAS_DEATH_TEST

    TYPED_TEST(TreeMapDifferentAllocatorFixture, SwapMovesElementsWhenAllocatorsDiffer)
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorMap = TypeParam::Create({ {1, 2}, {2, 4}, {3, 6} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        ContainerType osAllocatorMap = TypeParam::Create({ {2, 4} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Swap the container elements around while leaving the allocators the same.
        systemAllocatorMap.swap(osAllocatorMap);

        EXPECT_EQ(1, systemAllocatorMap.size());
        EXPECT_EQ(1, systemAllocatorMap.count(2));
        EXPECT_EQ(AZ::AZStdIAllocator(&AZ::AllocatorInstance<AZ::SystemAllocator>::Get()), systemAllocatorMap.get_allocator());

        EXPECT_EQ(3, osAllocatorMap.size());
        EXPECT_EQ(1, osAllocatorMap.count(1));
        EXPECT_EQ(1, osAllocatorMap.count(2));
        EXPECT_EQ(1, osAllocatorMap.count(3));
        EXPECT_EQ(AZ::AZStdIAllocator(&AZ::AllocatorInstance<AZ::OSAllocator>::Get()), osAllocatorMap.get_allocator());
    }

    namespace TreeContainerTransparentTestInternal
    {
        static int s_allConstructorCount;
        static int s_allAssignmentCount;

        struct TrackConstructorCalls
        {
            TrackConstructorCalls()
            {
                ++s_allConstructorCount;
            }
            TrackConstructorCalls(int value)
                : m_value(value)
            {
                ++s_allConstructorCount;
            }
            TrackConstructorCalls(const TrackConstructorCalls& other)
                : m_value(other.m_value)
            {
                ++s_allConstructorCount;
            };
            TrackConstructorCalls& operator=(const TrackConstructorCalls& other)
            {
                ++s_allAssignmentCount;
                m_value = other.m_value;
                return *this;
            }

            operator int() const
            {
                return m_value;
            }

            int m_value{}; // Used for comparison
        };

        bool operator<(const TrackConstructorCalls& lhs, const TrackConstructorCalls& rhs)
        {
            return lhs.m_value < rhs.m_value;
        }
        bool operator<(int lhs, const TrackConstructorCalls& rhs)
        {
            return lhs < rhs.m_value;
        }
        bool operator<(const TrackConstructorCalls& lhs, int rhs)
        {
            return lhs.m_value < rhs;
        }
    }

    template<typename Container>
    struct TreeContainerTransparentConfig
    {
        using ContainerType = Container;
    };

    template <typename ContainerType>
    class TreeContainerTransparentFixture
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            TreeContainerTransparentTestInternal::s_allConstructorCount = 0;
            TreeContainerTransparentTestInternal::s_allAssignmentCount = 0;
        }
        void TearDown() override
        {
            TreeContainerTransparentTestInternal::s_allConstructorCount = 0;
            TreeContainerTransparentTestInternal::s_allAssignmentCount = 0;
        }
    };

    using TreeContainerConfigs = ::testing::Types <
        TreeContainerTransparentConfig<AZStd::set<TreeContainerTransparentTestInternal::TrackConstructorCalls,  AZStd::less<>>>
        , TreeContainerTransparentConfig<AZStd::multiset<TreeContainerTransparentTestInternal::TrackConstructorCalls, AZStd::less<>>>
        , TreeContainerTransparentConfig<AZStd::map<TreeContainerTransparentTestInternal::TrackConstructorCalls, int, AZStd::less<>>>
        , TreeContainerTransparentConfig<AZStd::multimap<TreeContainerTransparentTestInternal::TrackConstructorCalls, int, AZStd::less<>>>
    >;

    TYPED_TEST_CASE(TreeContainerTransparentFixture, TreeContainerConfigs);

    TYPED_TEST(TreeContainerTransparentFixture, FindDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.find(TreeContainerTransparentTestInternal::TrackConstructorCalls{ 2 });
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // The transparent find function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.find(1);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // When find fails to locate an element, TrackConstructorCallsElement shouldn't be construction
        foundIt = container.find(-1);
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(container.end(), foundIt);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(TreeContainerTransparentFixture, ContainsDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        EXPECT_TRUE(container.contains(TreeContainerTransparentTestInternal::TrackConstructorCalls{ 2 }));
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);


        // The transparent contain function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        EXPECT_TRUE(container.contains(2));
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        EXPECT_FALSE(container.contains(27));
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(TreeContainerTransparentFixture, CountDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        EXPECT_EQ(1, container.count(TreeContainerTransparentTestInternal::TrackConstructorCalls{ 2 }));
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);

        // The transparent count function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        EXPECT_EQ(1, container.count(2));
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        EXPECT_EQ(0, container.count(27));
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(TreeContainerTransparentFixture, LowerBoundDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(21);
        container.emplace(4);
        container.emplace(3);
        container.emplace(7);
        container.emplace(15);
        container.emplace(42);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.lower_bound(TreeContainerTransparentTestInternal::TrackConstructorCalls{ 2 });
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // The transparent lower_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.lower_bound(26);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        foundIt = container.lower_bound(562);
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(container.end(), foundIt);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(TreeContainerTransparentFixture, UpperBoundDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(2);
        container.emplace(4);
        container.emplace(15);
        container.emplace(332);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.upper_bound(TreeContainerTransparentTestInternal::TrackConstructorCalls{ 3 });
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // The transparent upper_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.upper_bound(278);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        foundIt = container.upper_bound(1000);
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(container.end(), foundIt);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(TreeContainerTransparentFixture, EqualRangeDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(-2352);
        container.emplace(3534);
        container.emplace(535408957);
        container.emplace(1310556522);
        container.emplace(55546193);
        container.emplace(1582);

        // Reset constructor count to zero
        TreeContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundRange = container.equal_range(TreeContainerTransparentTestInternal::TrackConstructorCalls{ -2352 });
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(foundRange.first, foundRange.second);

        // The transparent upper_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundRange = container.equal_range(55546193);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(foundRange.first, foundRange.second);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        foundRange = container.equal_range(0x9034);
        EXPECT_EQ(1, TreeContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(foundRange.first, foundRange.second);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, TreeContainerTransparentTestInternal::s_allAssignmentCount);
    }
}

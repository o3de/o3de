/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/containers/fixed_list.h>
#include <AzCore/std/containers/fixed_forward_list.h>
#include <AzCore/std/functional.h>

#include <AzCore/std/containers/array.h>

#define AZ_TEST_VALIDATE_EMPTY_LIST(_List)        \
    EXPECT_TRUE(_List.empty());

#define AZ_TEST_VALIDATE_LIST(_List, _NumElements)                                                    \
    EXPECT_EQ(_NumElements, _List.size());                                                     \
    AZ_PUSH_DISABLE_WARNING_MSVC(4127) \
    if(_NumElements > 0) { EXPECT_FALSE(_List.empty()); } else { EXPECT_TRUE(_List.empty()); } \
    if(_NumElements > 0) { EXPECT_NE(_List.end(),_List.begin()); } else { EXPECT_EQ(_List.end(),_List.begin()); } \
    AZ_POP_DISABLE_WARNING_MSVC \
    EXPECT_FALSE(_List.empty());

namespace UnitTest
{
    template<class T, size_t NodeCount>
    auto FindPrevValidElementBefore(const AZStd::fixed_forward_list<T, NodeCount>& forwardList,
        typename AZStd::fixed_forward_list<T, NodeCount>::const_iterator last, int prevCount = 1)
    {
        // Find the previous valid element that is at least prevCount before
        // the @last iterator
        auto lastValidIter = forwardList.before_begin();
        for (auto first = AZStd::ranges::next(lastValidIter, prevCount);
            first != last; ++lastValidIter, ++first)
        {
        }

        return lastValidIter;
    };

    template<class T, size_t NodeCount>
    auto FindLastValidElementBefore(const AZStd::fixed_forward_list<T, NodeCount>& forwardList,
        typename AZStd::fixed_forward_list<T, NodeCount>::const_iterator last)
    {
        return FindPrevValidElementBefore(forwardList, last, 1);
    };

    class FixedListContainers
        : public LeakDetectionFixture
    {
    public:
        struct RemoveLessThan401
        {
            AZ_FORCE_INLINE bool operator()(int element) const { return element < 401; }
        };

        struct UniqueForLessThan401
        {
            AZ_FORCE_INLINE bool operator()(int el1, int el2) const { return (el1 == el2 && el1 < 401); }
        };
    };

    TEST_F(FixedListContainers, ListCtorAssign)
    {
        AZStd::fixed_list<int, 100> int_list;
        AZStd::fixed_list<int, 100> int_list1;
        AZStd::fixed_list<int, 100> int_list2;
        AZStd::fixed_list<int, 100> int_list3;
        AZStd::fixed_list<int, 100> int_list4;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);

        // 10 int elements, value 33
        int_list1 = AZStd::fixed_list<int, 100>(10, 33);
        AZ_TEST_VALIDATE_LIST(int_list1, 10);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list1.begin(); iter != int_list1.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy list 1 using, first,last,allocator.
        int_list2 = AZStd::fixed_list<int, 100>(int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list2, 10);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list2.begin(); iter != int_list2.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy construct
        int_list3 = AZStd::fixed_list<int, 100>(int_list1);
        EXPECT_EQ(int_list3, int_list1);

        // initializer_list construct
        int_list4 = { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33 };
        EXPECT_EQ(int_list4, int_list1);

        // assign
        int_list = int_list1;
        EXPECT_EQ(int_list1, int_list);

        int_list2.push_back(60);
        int_list = int_list2;
        EXPECT_EQ(int_list2, int_list);

        int_list2.pop_back();
        int_list2.pop_back();
        int_list = int_list2;
        EXPECT_EQ(int_list2, int_list);

        // assign with iterators (at the moment this uses the function operator= calls)
        int_list2.assign(int_list1.begin(), int_list1.end());
        EXPECT_EQ(int_list1, int_list2);

        // assign a fixed value
        int_list.assign(5, 55);
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list.begin(); iter != int_list.end(); ++iter)
        {
            EXPECT_EQ(55, *iter);
        }
    }

    TEST_F(FixedListContainers, ListResizeInsertErase)
    {
        AZStd::fixed_list<int, 100> int_list;
        AZStd::fixed_list<int, 100> int_list1;

        int_list.assign(5, 55);
        int_list1 = AZStd::fixed_list<int, 100>(10, 33);

        // resize to bigger
        int_list.resize(6, 43);
        AZ_TEST_VALIDATE_LIST(int_list, 6);
        EXPECT_EQ(43, int_list.back());
        EXPECT_EQ(55, *prev(int_list.end(), 2));
        EXPECT_EQ(*prev(int_list.rend()), *int_list.begin());

        // resize to smaller
        int_list.resize(3);
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        EXPECT_EQ(55, int_list.back());

        // insert
        int_list.insert(int_list.begin(), 44);
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        EXPECT_EQ(44, int_list.front());

        int_list.insert(int_list.end(), 66);
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        EXPECT_EQ(66, int_list.back());

        int_list.insert(int_list.begin(), 2, 11);
        AZ_TEST_VALIDATE_LIST(int_list, 7);
        EXPECT_EQ(11, int_list.front());
        EXPECT_EQ(11, *next(int_list.begin()));

        int_list.insert(int_list.end(), 2, 22);
        AZ_TEST_VALIDATE_LIST(int_list, 9);
        EXPECT_EQ(22, int_list.back());
        EXPECT_EQ(22, *prev(int_list.end(), 2));

        int_list.insert(int_list.end(), int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list, 9 + int_list1.size());
        EXPECT_EQ(33, int_list.back());

        // erase
        int_list.assign(2, 10);
        int_list.push_back(20);
        int_list.erase(--int_list.end());
        AZ_TEST_VALIDATE_LIST(int_list, 2);
        EXPECT_EQ(10, int_list.back());

        int_list.insert(int_list.end(), 3, 44);
        int_list.erase(prev(int_list.end(), 2), int_list.end());
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        EXPECT_EQ(44, int_list.back());

        // clear
        int_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);
    }

    TEST_F(FixedListContainers, ListSwapSplice)
    {
        AZStd::fixed_list<int, 100> int_list;
        AZStd::fixed_list<int, 100> int_list1;
        AZStd::fixed_list<int, 100> int_list2;

        int_list2 = AZStd::fixed_list<int, 100>(10, 33);

        // list operations with container with the same allocator.
        // swap
        int_list.swap(int_list2);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 10);

        int_list.swap(int_list2);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);
        AZ_TEST_VALIDATE_LIST(int_list2, 10);

        int_list.assign(5, 55);
        int_list.swap(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 10);
        AZ_TEST_VALIDATE_LIST(int_list2, 5);
        EXPECT_EQ(33, int_list.front());
        EXPECT_EQ(55, int_list2.front());

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_list.splice(int_list.end(), int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 15);
        EXPECT_EQ(33, int_list.front());
        EXPECT_EQ(55, int_list.back());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_list2.push_back(101);
        int_list.splice(int_list.begin(), int_list2, int_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 16);
        EXPECT_EQ(101, int_list.front());

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_list2.assign(5, 201);
        int_list.splice(int_list.end(), int_list2, ++int_list2.begin(), int_list2.end());
        AZ_TEST_VALIDATE_LIST(int_list2, 1);
        AZ_TEST_VALIDATE_LIST(int_list, 20);
        EXPECT_EQ(201, int_list.back());

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        int_list2.push_back(301);
        int_list.splice(int_list.end(), int_list2, int_list2.begin(), int_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 22);
        EXPECT_EQ(301, int_list.back());
    }

    TEST_F(FixedListContainers, ListRemoveUniqueSort)
    {
        AZStd::fixed_list<int, 100> int_list;
        
        // remove
        int_list.assign(5, 101);
        int_list.push_back(201);
        int_list.push_back(301);
        int_list.push_back(401);
        int_list.remove(101);
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        EXPECT_EQ(201, int_list.front());
        EXPECT_EQ(401, int_list.back());

        int_list.remove_if(RemoveLessThan401());
        AZ_TEST_VALIDATE_LIST(int_list, 1);
        EXPECT_EQ(401, int_list.back());

        // unique
        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(201);
        int_list.push_back(301);
        int_list.unique();
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        EXPECT_EQ(101, int_list.front());
        EXPECT_EQ(301, int_list.back());

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(201);
        int_list.push_back(401);
        int_list.push_back(401);
        int_list.push_back(501);
        int_list.unique(UniqueForLessThan401());
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        EXPECT_EQ(101, int_list.front());
        EXPECT_EQ(501, int_list.back());
        // sort
        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort();
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            EXPECT_GE(*iter, *next(iter));
        }
    }

    TEST_F(FixedListContainers, ListReverseMerge)
    {
        AZStd::fixed_list<int, 100> int_list;
        AZStd::fixed_list<int, 100> int_list1;
        AZStd::fixed_list<int, 100> int_list2;
        AZStd::fixed_list<int, 100> int_list3;

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());

        // reverse
        int_list.reverse();
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        // merge
        int_list.clear();
        int_list1.clear();
        int_list.push_back(1); // 2 sorted lists for merge
        int_list.push_back(10);
        int_list.push_back(50);
        int_list.push_back(200);
        int_list1.push_back(2);
        int_list1.push_back(8);
        int_list1.push_back(60);
        int_list1.push_back(180);

        int_list2 = int_list;
        int_list3 = int_list1;
        int_list2.merge(int_list3);
        AZ_TEST_VALIDATE_LIST(int_list2, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list3);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
        {
            EXPECT_LT(*iter, *next(iter));
        }

        int_list.reverse();
        int_list1.reverse();

        int_list2 = int_list;
        int_list3 = int_list1;
        int_list2.merge(int_list3, AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_list2, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list3);
        for (AZStd::fixed_list<int, 100>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
        {
            EXPECT_GT(*iter, *next(iter));
        }
    }

    TEST_F(FixedListContainers, ListExtensions)
    {
        AZStd::fixed_list<int, 100> int_list;

        // Push_back()
        int_list.emplace_back();
        AZ_TEST_VALIDATE_LIST(int_list, 1);
        int_list.front() = 100;

        // Push_front()
        int_list.emplace_front();
        AZ_TEST_VALIDATE_LIST(int_list, 2);
        EXPECT_EQ(100, int_list.back());

        // Insert without value to copy from.
        int_list.insert(int_list.begin());
        AZ_TEST_VALIDATE_LIST(int_list, 3);

        // default int alignment
        EXPECT_EQ(0, ((AZStd::size_t)&int_list.front() % 4)); // default int alignment

        // make sure every allocation is aligned.
        AZStd::fixed_list<UnitTestInternal::MyClass, 100> aligned_list(5, UnitTestInternal::MyClass(99));
        EXPECT_EQ(0, ((AZStd::size_t)&aligned_list.front() & (alignof(UnitTestInternal::MyClass) - 1)));
    }

    TEST_F(FixedListContainers, ForwardListCtorAssign)
    {
        AZStd::fixed_forward_list<int, 100> int_slist;
        AZStd::fixed_forward_list<int, 100> int_slist1;
        AZStd::fixed_forward_list<int, 100> int_slist2;
        AZStd::fixed_forward_list<int, 100> int_slist3;
        AZStd::fixed_forward_list<int, 100> int_slist4;

        // default ctor

        // 10 int elements, value 33
        int_slist1 = AZStd::fixed_forward_list<int, 100>(10, 33);
        for (AZStd::fixed_forward_list<int, 100>::iterator iter = int_slist1.begin(); iter != int_slist1.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy list 1 using, first,last,allocator.
        int_slist2 = AZStd::fixed_forward_list<int, 100>(int_slist1.begin(), int_slist1.end());
        for (AZStd::fixed_forward_list<int, 100>::iterator iter = int_slist2.begin(); iter != int_slist2.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy construct
        int_slist3 = AZStd::fixed_forward_list<int, 100>(int_slist1);
        EXPECT_EQ(int_slist3, int_slist1);

        // initializer_list construct
        int_slist4 = { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33 };
        EXPECT_EQ(int_slist4, int_slist1);

        // assign
        int_slist = int_slist1;
        EXPECT_EQ(int_slist1, int_slist);

        int_slist2.emplace_after(FindLastValidElementBefore(int_slist2, int_slist2.end()), 60);
        int_slist = int_slist2;
        EXPECT_EQ(int_slist2, int_slist);

        //int_slist2.pop_back();
        //int_slist2.pop_back();
        //int_slist = int_slist2;
        //EXPECT_EQ(int_slist2, int_slist);

        // assign with iterators (at the moment this uses the function operator= calls)
        int_slist2.assign(int_slist1.begin(), int_slist1.end());
        EXPECT_EQ(int_slist1, int_slist2);

        // assign a fixed value
        int_slist.assign(5, 55);
        for (AZStd::fixed_forward_list<int, 100>::iterator iter = int_slist.begin(); iter != int_slist.end(); ++iter)
        {
            EXPECT_EQ(55, *iter);
        }
    }

    TEST_F(FixedListContainers, ForwardListResizeInsertErase)
    {
        AZStd::fixed_forward_list<int, 100> int_slist;
        AZStd::fixed_forward_list<int, 100> int_slist1;
       
        int_slist.assign(5, 55);
        int_slist1 = AZStd::fixed_forward_list<int, 100>(10, 33);

        // resize to bigger
        int_slist.resize(6, 43);
        EXPECT_EQ(43, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // resize to smaller
        int_slist.resize(3);
        EXPECT_EQ(55, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // insert
        int_slist.insert_after(int_slist.before_begin(), 44);
        EXPECT_EQ(44, int_slist.front());

        int_slist.insert_after(FindLastValidElementBefore(int_slist, int_slist.end()), 66);
        EXPECT_EQ(66, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.insert_after(int_slist.before_begin(), 2, 11);
        EXPECT_EQ(11, int_slist.front());
        EXPECT_EQ(11, *next(int_slist.begin()));

        int_slist.insert_after(FindLastValidElementBefore(int_slist, int_slist.end()), 2, 22);
        EXPECT_EQ(22, *FindLastValidElementBefore(int_slist, int_slist.end()));
        EXPECT_EQ(22, *FindLastValidElementBefore(int_slist, FindLastValidElementBefore(int_slist, int_slist.end())));

        int_slist.insert_after(int_slist.before_end(), int_slist1.begin(), int_slist1.end());
        EXPECT_EQ(33, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // erase
        int_slist.assign(2, 10);
        int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 20);
        auto secondToLastElementIter = FindPrevValidElementBefore(int_slist, int_slist.end(), 2);
        int_slist.erase_after(secondToLastElementIter);
        EXPECT_EQ(10, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.insert_after(FindLastValidElementBefore(int_slist, int_slist.end()), 3, 44);
        auto thirdToLastElementIter = FindPrevValidElementBefore(int_slist, int_slist.end(), 3);
        int_slist.erase_after(thirdToLastElementIter);
        EXPECT_EQ(44, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // clear
        int_slist.clear();
    }

    TEST_F(FixedListContainers, ForwardListSwapSplice)
    {
        AZStd::fixed_forward_list<int, 100> int_slist;
        AZStd::fixed_forward_list<int, 100> int_slist1;
        AZStd::fixed_forward_list<int, 100> int_slist2;

        int_slist2 = AZStd::fixed_forward_list<int, 100>(10, 33);

        // list operations with container with the same allocator.
        // swap
        int_slist.swap(int_slist2);
        EXPECT_TRUE(int_slist2.empty());

        int_slist.swap(int_slist2);
        EXPECT_TRUE(int_slist.empty());
        EXPECT_FALSE(int_slist2.empty());

        int_slist.assign(5, 55);
        int_slist.swap(int_slist2);
        EXPECT_EQ(33, int_slist.front());
        EXPECT_EQ(55, int_slist2.front());

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_slist.splice_after(FindLastValidElementBefore(int_slist, int_slist.end()), int_slist2);
        EXPECT_EQ(33, int_slist.front());
        EXPECT_EQ(55, *FindLastValidElementBefore(int_slist, int_slist.end()));
        EXPECT_TRUE(int_slist2.empty());

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_slist2.emplace_after(FindLastValidElementBefore(int_slist2, int_slist2.end()), 101);
        int_slist.splice_after(int_slist.before_begin(), int_slist2, int_slist2.before_begin());
        EXPECT_EQ(101, int_slist.front());

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_slist2.assign(5, 201);
        int_slist.splice_after(FindLastValidElementBefore(int_slist, int_slist.end()), int_slist2, int_slist2.begin(), int_slist2.end());
        EXPECT_EQ(201, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        int_slist2.emplace_after(FindLastValidElementBefore(int_slist2, int_slist2.end()), 301);
        int_slist.splice_after(FindLastValidElementBefore(int_slist, int_slist.end()), int_slist2, int_slist2.before_begin(), int_slist2.end());
        EXPECT_EQ(301, *FindLastValidElementBefore(int_slist, int_slist.end()));
    }

    TEST_F(FixedListContainers, ForwardListRemoveUniqueSort)
    {
        AZStd::fixed_forward_list<int, 100> int_slist;

        // remove
        int_slist.assign(5, 101);
        auto lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 301);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 401);
        int_slist.remove(101);
        EXPECT_EQ(201, int_slist.front());
        EXPECT_EQ(401, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.remove_if(RemoveLessThan401());
        EXPECT_EQ(401, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // unique
        int_slist.assign(2, 101);
        lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 301);
        int_slist.unique();
        EXPECT_EQ(101, int_slist.front());
        EXPECT_EQ(301, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.assign(2, 101);
        lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 401);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 401);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 501);
        int_slist.unique(UniqueForLessThan401());
        EXPECT_EQ(101, int_slist.front());
        EXPECT_EQ(501, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // sort
        int_slist.assign(2, 101);
        lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 1);
        int_slist.sort();
        for (auto iter = int_slist.begin(); iter != FindLastValidElementBefore(int_slist, int_slist.end()); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        int_slist.assign(2, 101);
        lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 1);
        int_slist.sort(AZStd::greater<int>());
        for (auto iter = int_slist.begin(); iter != FindLastValidElementBefore(int_slist, int_slist.end()); ++iter)
        {
            EXPECT_GE(*iter, *next(iter));
        }
    }

    TEST_F(FixedListContainers, ForwardListReverseMerge)
    {
        AZStd::fixed_forward_list<int, 100> int_slist;
        AZStd::fixed_forward_list<int, 100> int_slist1;
        AZStd::fixed_forward_list<int, 100> int_slist2;
        AZStd::fixed_forward_list<int, 100> int_slist3;

        int_slist.assign(2, 101);
        auto lastInsertIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastInsertIter = int_slist.emplace_after(lastInsertIter, 1);
        int_slist.sort(AZStd::greater<int>());

        // reverse
        int_slist.reverse();
        for (auto iter = int_slist.begin(); iter != FindLastValidElementBefore(int_slist, int_slist.end()); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        // merge
        int_slist.clear();
        int_slist1.clear();
        // 2 sorted lists for merge
        {
            lastInsertIter = int_slist.emplace_after(int_slist.before_begin(), 1);
            lastInsertIter = int_slist.emplace_after(lastInsertIter, 10);
            lastInsertIter = int_slist.emplace_after(lastInsertIter, 50);
            lastInsertIter = int_slist.emplace_after(lastInsertIter, 200);
        }
        {
            lastInsertIter = int_slist1.emplace_after(int_slist1.before_begin(), 2);
            lastInsertIter = int_slist1.emplace_after(lastInsertIter, 8);
            lastInsertIter = int_slist1.emplace_after(lastInsertIter, 60);
            lastInsertIter = int_slist1.emplace_after(lastInsertIter, 180);
        }

        int_slist2 = int_slist;
        int_slist3 = int_slist1;
        int_slist2.merge(int_slist3);
        for (auto iter = int_slist2.begin(); iter != FindLastValidElementBefore(int_slist2, int_slist2.end()); ++iter)
        {
            EXPECT_LT(*iter, *next(iter));
        }

        int_slist.reverse();
        int_slist1.reverse();

        int_slist2 = int_slist;
        int_slist3 = int_slist1;
        int_slist2.merge(int_slist3, AZStd::greater<int>());
        for (auto iter = int_slist2.begin(); iter != FindLastValidElementBefore(int_slist2, int_slist2.end()); ++iter)
        {
            EXPECT_GT(*iter, *next(iter));
        }
    }
}

#undef AZ_TEST_VALIDATE_EMPTY_LIST
#undef AZ_TEST_VALIDATE_LIST

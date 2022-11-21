/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/allocator_ref.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/ranges/transform_view.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#define AZ_TEST_VALIDATE_EMPTY_LIST(_List) \
    EXPECT_TRUE(_List.empty());

#define AZ_TEST_VALIDATE_LIST(_List, _NumElements) \
    EXPECT_EQ(_NumElements, _List.size()); \
    AZ_PUSH_DISABLE_WARNING_MSVC(4127) \
    if(_NumElements > 0) { EXPECT_FALSE(_List.empty()); } else { EXPECT_TRUE(_List.empty()); } \
    if(_NumElements > 0) { EXPECT_NE(_List.end(),_List.begin()); } else { EXPECT_EQ(_List.end(),_List.begin()); } \
    AZ_POP_DISABLE_WARNING_MSVC \

namespace UnitTest
{
    template<class T, class Alloc>
    auto FindPrevValidElementBefore(const AZStd::forward_list<T, Alloc>& forwardList,
        typename AZStd::forward_list<T, Alloc>::const_iterator last, size_t prevCount = 1)
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

    template<class T, class Alloc>
    auto FindLastValidElementBefore(const AZStd::forward_list<T, Alloc>& forwardList,
        typename AZStd::forward_list<T, Alloc>::const_iterator last)
    {
        return FindPrevValidElementBefore(forwardList, last, 1);
    };

    /**
     * Tests AZSTD::list container.
     */
    class ListContainers
        : public AllocatorsFixture
    {
    public:
        // ListContainerTest-Begin
        struct RemoveLessThan401
        {
            AZ_FORCE_INLINE bool operator()(int element) const { return element < 401; }
        };

        struct UniqueForLessThan401
        {
            AZ_FORCE_INLINE bool operator()(int el1, int el2) const { return (el1 == el2 && el1 < 401); }
        };
    };

    TEST_F(ListContainers, InitializerListCtor)
    {
        AZStd::list<int> intList({ 1, 2, 3, 4, 5, 6 });
        EXPECT_EQ(6, intList.size());
    }

    TEST_F(ListContainers, ListCtorAssign)
    {
        AZStd::list<int>   int_list;
        AZStd::list<int>   int_list1;
        AZStd::list<int>   int_list2;
        AZStd::list<int>   int_list3;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);

        // 10 int elements, value 33
        int_list1 = AZStd::list<int>(10, 33);
        AZ_TEST_VALIDATE_LIST(int_list1, 10);
        for (AZStd::list<int>::iterator iter = int_list1.begin(); iter != int_list1.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy list 1 using, first,last,allocator.
        int_list2 = AZStd::list<int>(int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list2, 10);
        for (AZStd::list<int>::iterator iter = int_list2.begin(); iter != int_list2.end(); ++iter)
        {
            EXPECT_EQ(33, *iter);
        }

        // copy construct
        int_list3 = AZStd::list<int>(int_list1);
        EXPECT_EQ(int_list3, int_list1);

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
        for (AZStd::list<int>::iterator iter = int_list.begin(); iter != int_list.end(); ++iter)
        {
            EXPECT_EQ(55, *iter);
        }
    }

    TEST_F(ListContainers, ListResizeInsertEraseClear)
    {
        AZStd::list<int>   int_list;
        AZStd::list<int>   int_list1;
        AZStd::list<int>   int_list2;
        AZStd::list<int>   int_list3;
        AZStd::list<int>::iterator list_it;

        int_list.assign(5, 55);
        int_list1 = AZStd::list<int>(10, 33);

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
        list_it = int_list.insert(int_list.begin(), 44);
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        EXPECT_EQ(44, int_list.front());
        EXPECT_EQ(44, *list_it);

        list_it = int_list.insert(int_list.end(), 66);
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        EXPECT_EQ(66, int_list.back());
        EXPECT_EQ(66, *list_it);

        list_it = int_list.insert(int_list.begin(), 2, 11);
        AZ_TEST_VALIDATE_LIST(int_list, 7);
        EXPECT_EQ(11, int_list.front());
        EXPECT_EQ(11, *next(int_list.begin()));
        EXPECT_EQ(11, *list_it);
        EXPECT_EQ(11, *next(list_it));

        list_it = int_list.insert(int_list.end(), 2, 22);
        AZ_TEST_VALIDATE_LIST(int_list, 9);
        EXPECT_EQ(22, int_list.back());
        EXPECT_EQ(22, *prev(int_list.end(), 2));
        EXPECT_EQ(66, *prev(list_it)); // 66 was the last element added at int_list.end()
        EXPECT_EQ(22, *list_it);
        EXPECT_EQ(22, *next(list_it));

        list_it = int_list.insert(int_list.end(), int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list, 9 + int_list1.size());
        EXPECT_EQ(33, int_list.back());

        // Ensure that parallel iterating over int_list1 and int_list
        // from the returned iterator result in the same elements.
        AZStd::list<int>::iterator list1_it = int_list1.begin();
        for (; list1_it != int_list1.end(); ++list_it, ++list1_it)
        {
            EXPECT_NE(int_list.end(), list_it);
            EXPECT_NE(int_list1.end(), list1_it);
            EXPECT_EQ(*list_it, *list_it);
        }

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

    TEST_F(ListContainers, SwapSplice)
    {
        AZStd::list<int>   int_list;
        AZStd::list<int>   int_list1;
        AZStd::list<int>   int_list2;

        int_list2 = AZStd::list<int>(10, 33);

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

    TEST_F(ListContainers, RemoveUniqueSort)
    {
        AZStd::list<int>   int_list;

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
        for (AZStd::list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        for (AZStd::list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            EXPECT_GE(*iter, *next(iter));
        }
    }

    TEST_F(ListContainers, ReverseMerge)
    {
        AZStd::list<int>   int_list;
        AZStd::list<int>   int_list1;
        AZStd::list<int>   int_list2;
        AZStd::list<int>   int_list3;

        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());

        // reverse
        int_list.reverse();
        for (AZStd::list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
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
        for (AZStd::list<int>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
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
        for (AZStd::list<int>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
        {
            EXPECT_GT(*iter, *next(iter));
        }
    }

    TEST_F(ListContainers, Extensions)
    {
        AZStd::list<int>   int_list;
        // Extensions.
        int_list.clear();

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
        AZStd::list<UnitTestInternal::MyClass> aligned_list(5, UnitTestInternal::MyClass(99));
        EXPECT_EQ(0, ((AZStd::size_t)&aligned_list.front() & (alignof(UnitTestInternal::MyClass) - 1)));
    }

    TEST_F(ListContainers, StaticBufferAllocator)
    {
        /////////////////////////////////////////////////////////////////////////
        // Test swap, splice, merge, etc. of containers with different allocators.
        // list operations with container with the same allocator.
        using static_buffer_16KB_type = AZStd::static_buffer_allocator<16 * 1024, 1>;
        static_buffer_16KB_type myMemoryManager1;
        static_buffer_16KB_type myMemoryManager2;
        using static_allocator_ref_type = AZStd::allocator_ref<static_buffer_16KB_type>;
        static_allocator_ref_type allocator1(myMemoryManager1);
        static_allocator_ref_type allocator2(myMemoryManager2);

        using stack_myclass_list_type = AZStd::list<UnitTestInternal::MyClass, static_allocator_ref_type>;
        stack_myclass_list_type int_list10(allocator1);
        stack_myclass_list_type int_list20(allocator2);

        int_list20.assign(10, UnitTestInternal::MyClass(33));
        AZ_TEST_VALIDATE_LIST(int_list20, 10);
        EXPECT_GE(myMemoryManager2.get_allocated_size(), 10 * sizeof(stack_myclass_list_type::node_type));

        int_list20.leak_and_reset();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        myMemoryManager2.reset(); // free all memory

        // set allocator
        int_list20.assign(20, UnitTestInternal::MyClass(22));
        int_list20.set_allocator(allocator1);
        AZ_TEST_VALIDATE_LIST(int_list20, 20);
        EXPECT_GE(myMemoryManager1.get_allocated_size(), 20 * sizeof(stack_myclass_list_type::node_type));
        EXPECT_LE(myMemoryManager2.get_allocated_size(), 20 * sizeof(stack_myclass_list_type::node_type));
        int_list20.leak_and_reset();
        int_list20.set_allocator(allocator2);
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        int_list20.assign(10, UnitTestInternal::MyClass(11));

        // swap
        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 10);
        EXPECT_GE(myMemoryManager1.get_allocated_size(), 10 * sizeof(stack_myclass_list_type::node_type));
        EXPECT_LE(myMemoryManager2.get_allocated_size(), 10 * sizeof(stack_myclass_list_type::node_type));

        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list10);
        AZ_TEST_VALIDATE_LIST(int_list20, 10);

        int_list10.assign(5, 55);
        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 10);
        AZ_TEST_VALIDATE_LIST(int_list20, 5);
        EXPECT_EQ(UnitTestInternal::MyClass(11), int_list10.front());
        EXPECT_EQ(UnitTestInternal::MyClass(55), int_list20.front());

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_list10.splice(int_list10.end(), int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 15);
        EXPECT_EQ(UnitTestInternal::MyClass(11), int_list10.front());
        EXPECT_EQ(UnitTestInternal::MyClass(55), int_list10.back());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_list20.push_back(UnitTestInternal::MyClass(101));
        int_list10.splice(int_list10.begin(), int_list20, int_list20.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 16);
        EXPECT_EQ(UnitTestInternal::MyClass(101), int_list10.front());

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_list20.assign(5, UnitTestInternal::MyClass(201));
        int_list10.splice(int_list10.end(), int_list20, ++int_list20.begin(), int_list20.end());
        AZ_TEST_VALIDATE_LIST(int_list20, 1);
        AZ_TEST_VALIDATE_LIST(int_list10, 20);
        EXPECT_EQ(UnitTestInternal::MyClass(201), int_list10.back());

        int_list10.leak_and_reset();
        int_list20.leak_and_reset();
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        // merge
        stack_myclass_list_type int_list30(allocator1);
        stack_myclass_list_type int_list40(allocator2);
        int_list10.push_back(UnitTestInternal::MyClass(1)); // 2 sorted lists for merge
        int_list10.push_back(UnitTestInternal::MyClass(10));
        int_list10.push_back(UnitTestInternal::MyClass(50));
        int_list10.push_back(UnitTestInternal::MyClass(200));
        int_list20.push_back(UnitTestInternal::MyClass(2));
        int_list20.push_back(UnitTestInternal::MyClass(8));
        int_list20.push_back(UnitTestInternal::MyClass(60));
        int_list20.push_back(UnitTestInternal::MyClass(180));

        int_list30 = int_list10;
        int_list40 = int_list20;
        int_list30.merge(int_list40);
        AZ_TEST_VALIDATE_LIST(int_list30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list40);
        for (stack_myclass_list_type::iterator iter = int_list30.begin(); iter != --int_list30.end(); ++iter)
        {
            EXPECT_LT(*iter, *next(iter));
        }

        int_list10.reverse();
        int_list20.reverse();

        int_list30 = int_list10;
        int_list40 = int_list20;
        int_list30.merge(int_list40, AZStd::greater<UnitTestInternal::MyClass>());
        AZ_TEST_VALIDATE_LIST(int_list30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list40);
        for (stack_myclass_list_type::iterator iter = int_list30.begin(); iter != --int_list30.end(); ++iter)
        {
            EXPECT_GT(*iter, *next(iter));
        }
    }

    TEST_F(ListContainers, UniquePtr)
    {
        AZStd::list<AZStd::unique_ptr<UnitTestInternal::MyNoCopyClass>> nocopy_list;
        nocopy_list.emplace_back(new UnitTestInternal::MyNoCopyClass(1, true, 3.0f));
        nocopy_list.emplace_front(new UnitTestInternal::MyNoCopyClass(2, true, 4.0f));
        nocopy_list.emplace(nocopy_list.end(), new UnitTestInternal::MyNoCopyClass(3, true, 5.0f));

        nocopy_list.insert(nocopy_list.end(), AZStd::unique_ptr<UnitTestInternal::MyNoCopyClass>(new UnitTestInternal::MyNoCopyClass(4, true, 6.0f)));

        for (const auto& ptr : nocopy_list)
        {
            EXPECT_TRUE(ptr->m_bool);
        }
    }

    TEST_F(ListContainers, ForwardListAssign)
    {
        AZStd::forward_list<int> int_slist;
        AZStd::forward_list<int> int_slist1;
        AZStd::forward_list<int> int_slist2;
        AZStd::forward_list<int> int_slist3;
        AZStd::forward_list<int> int_slist4;

        // default ctor

        // 10 int elements, value 33
        int_slist1 = AZStd::forward_list<int>(10, 33);
        EXPECT_THAT(int_slist1, ::testing::Each(33));

        // copy list 1 using, first,last,allocator.
        int_slist2 = AZStd::forward_list<int>(int_slist1.begin(), int_slist1.end());
        EXPECT_THAT(int_slist2, ::testing::Each(33));

        // copy construct
        int_slist3 = AZStd::forward_list<int>(int_slist1);
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

        // assign with iterators
        int_slist2.assign(int_slist1.begin(), int_slist1.end());
        EXPECT_EQ(int_slist1, int_slist2);

        // assign a fixed value
        int_slist.assign(5, 55);
        EXPECT_THAT(int_slist, ::testing::Each(55));
    }

    TEST_F(ListContainers, ForwardListInsertEraseClear)
    {
        AZStd::forward_list<int> int_slist;
        AZStd::forward_list<int> int_slist1;

        int_slist.assign(5, 55);
        int_slist1 = AZStd::forward_list<int>(10, 33);

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
        auto testIter = FindLastValidElementBefore(int_slist, int_slist.end());
        EXPECT_EQ(22, *testIter);
        EXPECT_EQ(22, *FindLastValidElementBefore(int_slist, testIter));

        int_slist.insert_after(FindLastValidElementBefore(int_slist, int_slist.end()), int_slist1.begin(), int_slist1.end());
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

    TEST_F(ListContainers, ForwardListSwapSplice)
    {
        AZStd::forward_list<int> int_slist;
        AZStd::forward_list<int> int_slist1;
        AZStd::forward_list<int> int_slist2;

        int_slist2 = AZStd::forward_list<int>(10, 33);

        // list operations with container with the same allocator.
        // swap
        int_slist.swap(int_slist2);

        int_slist.swap(int_slist2);

        int_slist.assign(5, 55);
        int_slist.swap(int_slist2);
        EXPECT_EQ(33, int_slist.front());
        EXPECT_EQ(55, int_slist2.front());

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_slist.splice_after(FindLastValidElementBefore(int_slist, int_slist.end()), int_slist2);
        EXPECT_EQ(33, int_slist.front() );
        EXPECT_EQ(55, *FindLastValidElementBefore(int_slist, int_slist.end()));

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

    TEST_F(ListContainers, ForwardListRemoveUniqueSort)
    {
        AZStd::forward_list<int> int_slist;

        // remove
        int_slist.assign(5, 101);
        auto lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 301);
        lastValidIter = int_slist.emplace_after(lastValidIter, 401);
        int_slist.remove(101);
        EXPECT_EQ(201, int_slist.front());
        EXPECT_EQ(401, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.remove_if(RemoveLessThan401());
        EXPECT_EQ(401, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // unique
        int_slist.assign(2, 101);
        lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 301);
        int_slist.unique();
        EXPECT_EQ(101, int_slist.front());
        EXPECT_EQ(301, *FindLastValidElementBefore(int_slist, int_slist.end()));

        int_slist.assign(2, 101);
        lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 401);
        lastValidIter = int_slist.emplace_after(lastValidIter, 401);
        lastValidIter = int_slist.emplace_after(lastValidIter, 501);
        int_slist.unique(UniqueForLessThan401());
        EXPECT_EQ(101, int_slist.front());
        EXPECT_EQ(501, *FindLastValidElementBefore(int_slist, int_slist.end()));

        // sort
        int_slist.assign(2, 101);
        lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 1);
        int_slist.sort();
        for (auto iter = int_slist.begin(); iter != FindLastValidElementBefore(int_slist, int_slist.end()); ++iter)
        {
            EXPECT_LE(*iter, *next(iter));
        }

        int_slist.assign(2, 101);
        lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 1);
        int_slist.sort(AZStd::greater<int>());
        for (auto iter = int_slist.begin(); iter != FindLastValidElementBefore(int_slist, int_slist.end()); ++iter)
        {
            EXPECT_GE(*iter, *next(iter));
        }
    }

    TEST_F(ListContainers, ForwardListReverseMerge)
    {
        AZStd::forward_list<int> int_slist;
        AZStd::forward_list<int> int_slist1;
        AZStd::forward_list<int> int_slist2;
        AZStd::forward_list<int> int_slist3;

        int_slist.assign(2, 101);
        auto lastValidIter = int_slist.emplace_after(FindLastValidElementBefore(int_slist, int_slist.end()), 201);
        lastValidIter = int_slist.emplace_after(lastValidIter, 1);
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
            lastValidIter = int_slist.emplace_after(int_slist.before_begin(), 1);
            lastValidIter = int_slist.emplace_after(lastValidIter, 10);
            lastValidIter = int_slist.emplace_after(lastValidIter, 50);
            lastValidIter = int_slist.emplace_after(lastValidIter, 200);
        }
        {
            lastValidIter = int_slist1.emplace_after(int_slist1.before_begin(), 2);
            lastValidIter = int_slist1.emplace_after(lastValidIter, 8);
            lastValidIter = int_slist1.emplace_after(lastValidIter, 60);
            lastValidIter = int_slist1.emplace_after(lastValidIter, 180);
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

    TEST_F(ListContainers, ForwardListStaticBuffer)
    {
        // Test swap, splice, merge, etc. of containers with different allocators.
        // list operations with container with the same allocator.
        using static_buffer_16KB_type = AZStd::static_buffer_allocator<16 * 1024, 1>;
        static_buffer_16KB_type myMemoryManager1;
        static_buffer_16KB_type myMemoryManager2;
        using static_allocator_ref_type = AZStd::allocator_ref<static_buffer_16KB_type>;
        static_allocator_ref_type allocator1(myMemoryManager1);
        static_allocator_ref_type allocator2(myMemoryManager2);

        using stack_myclass_slist_type = AZStd::forward_list<UnitTestInternal::MyClass, static_allocator_ref_type>;
        stack_myclass_slist_type   int_slist10(allocator1);
        stack_myclass_slist_type   int_slist20(allocator2);

        constexpr size_t sizeofNode = sizeof(stack_myclass_slist_type::iterator::node_type);
        constexpr size_t alignofNode = alignof(stack_myclass_slist_type::iterator::node_type);
        // This assumes that the static buffer is the first field of the allocator. The first allocation will have to be
        // offset by the alignment of the list's node type
        const size_t offset1 = AZ::SizeAlignUp(reinterpret_cast<size_t>(&myMemoryManager1), alignofNode) - reinterpret_cast<size_t>(&myMemoryManager1);
        const size_t offset2 = AZ::SizeAlignUp(reinterpret_cast<size_t>(&myMemoryManager2), alignofNode) - reinterpret_cast<size_t>(&myMemoryManager2);

        int_slist20.assign(10, UnitTestInternal::MyClass(33));
        EXPECT_EQ(int_slist20.get_allocator().get_allocated_size(), offset2 + 10 * sizeofNode);

        int_slist20.clear();
        myMemoryManager2.reset();  // free all memory
        EXPECT_EQ(int_slist20.get_allocator().get_allocated_size(), 0);

        int_slist20.assign(10, UnitTestInternal::MyClass(11));
        EXPECT_EQ(int_slist20.get_allocator().get_allocated_size(), offset2 + 10 * sizeofNode);

        // swap
        EXPECT_EQ(int_slist10.get_allocator().get_allocated_size(), 0);
        int_slist10.swap(int_slist20);
        EXPECT_EQ(int_slist10.get_allocator().get_allocated_size(), offset1 + 10 * sizeofNode);
        // int_slist20's allocator only decreases by 1 object's worth of space, because the static buffer allocator can
        // only free from the end of its buffer
        EXPECT_EQ(int_slist20.get_allocator().get_allocated_size(), offset2 + 9 * sizeofNode);

        int_slist10.swap(int_slist20);

        int_slist10.assign(5, 55);
        int_slist10.swap(int_slist20);
        EXPECT_EQ(UnitTestInternal::MyClass(11), int_slist10.front());
        EXPECT_EQ(UnitTestInternal::MyClass(55), int_slist20.front());

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_slist10.splice_after(FindLastValidElementBefore(int_slist10, int_slist10.end()), int_slist20);
        EXPECT_EQ(UnitTestInternal::MyClass(11), int_slist10.front());
        EXPECT_EQ(UnitTestInternal::MyClass(55), *FindLastValidElementBefore(int_slist10, int_slist10.end()));

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_slist20.emplace_after(FindLastValidElementBefore(int_slist20, int_slist20.end()), UnitTestInternal::MyClass(101));
        int_slist10.splice_after(int_slist10.before_begin(), int_slist20, int_slist20.before_begin());
        EXPECT_EQ(UnitTestInternal::MyClass(101), int_slist10.front() );

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_slist20.assign(5, UnitTestInternal::MyClass(201));
        int_slist10.splice_after(FindLastValidElementBefore(int_slist10, int_slist10.end()), int_slist20, int_slist20.begin(), int_slist20.end());
        EXPECT_EQ(UnitTestInternal::MyClass(201), *FindLastValidElementBefore(int_slist10, int_slist10.end()));

        int_slist10.clear();
        int_slist20.clear();
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        // merge
        stack_myclass_slist_type   int_slist30(myMemoryManager1);
        stack_myclass_slist_type   int_slist40(myMemoryManager2);
        {
            auto lastValidIter = int_slist10.emplace_after(FindLastValidElementBefore(int_slist10, int_slist10.end()), UnitTestInternal::MyClass(1));  // 2 sorted lists for merge
            lastValidIter = int_slist10.emplace_after(lastValidIter, UnitTestInternal::MyClass(10));
            lastValidIter = int_slist10.emplace_after(lastValidIter, UnitTestInternal::MyClass(50));
            lastValidIter = int_slist10.emplace_after(lastValidIter, UnitTestInternal::MyClass(200));
        }
        {
            auto lastValidIter = int_slist20.emplace_after(FindLastValidElementBefore(int_slist20, int_slist20.end()), UnitTestInternal::MyClass(2));
            lastValidIter = int_slist20.emplace_after(lastValidIter, UnitTestInternal::MyClass(8));
            lastValidIter = int_slist20.emplace_after(lastValidIter, UnitTestInternal::MyClass(60));
            lastValidIter = int_slist20.emplace_after(lastValidIter, UnitTestInternal::MyClass(180));
        }

        int_slist30 = int_slist10;
        int_slist40 = int_slist20;
        int_slist30.merge(int_slist40);
        for (auto iter = int_slist30.begin(); iter != FindLastValidElementBefore(int_slist30, int_slist30.end()); ++iter)
        {
            EXPECT_LT(*iter, *next(iter));
        }

        int_slist10.reverse();
        int_slist20.reverse();

        int_slist30 = int_slist10;
        int_slist40 = int_slist20;
        int_slist30.merge(int_slist40, AZStd::greater<UnitTestInternal::MyClass>());
        for (auto iter = int_slist30.begin(); iter != FindLastValidElementBefore(int_slist30, int_slist30.end()); ++iter)
        {
            EXPECT_GT(*iter, *next(iter));
        }
    }

    template<template<class...> class ListTemplate>
    static void ListDeductionGuideSucceeds()
    {
        constexpr AZStd::string_view testView;
        ListTemplate testList(testView.begin(), testView.end());
        EXPECT_TRUE(testList.empty());

        testList = ListTemplate(AZStd::from_range, testView);
        EXPECT_TRUE(testList.empty());
    }

    TEST_F(ListContainers, DeductionGuides_Compiles)
    {
        ListDeductionGuideSucceeds<AZStd::list>();
        ListDeductionGuideSucceeds<AZStd::forward_list>();
    }

    template<template<class...> class ListTemplate>
    static void ListRangeConstructorSucceeds()
    {
        constexpr AZStd::string_view testView = "abc";

        ListTemplate testList(AZStd::from_range, testView);
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));

        testList = ListTemplate(AZStd::from_range, AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::forward_list<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::deque<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::unordered_set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::fixed_vector<char, 8>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::array{ 'a', 'b', 'c' });
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));

        AZStd::fixed_string<8> testValue(testView);
        testList = ListTemplate(AZStd::from_range, testValue);
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));
        testList = ListTemplate(AZStd::from_range, AZStd::string(testView));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c'));

        // Test Range views
        testList = ListTemplate(AZStd::from_range, testValue | AZStd::views::transform([](const char elem) -> char { return elem + 1; }));
        EXPECT_THAT(testList, ::testing::ElementsAre('b', 'c', 'd'));
    }

    TEST_F(ListContainers, RangeConstructors_Succeeds)
    {
        ListRangeConstructorSucceeds<AZStd::list>();
        ListRangeConstructorSucceeds<AZStd::forward_list>();
    }

    template<template<class...> class ListTemplate>
    static void ListAssignRangeSucceeds()
    {
        constexpr AZStd::string_view testView = "def";
        ListTemplate testList{ 'a', 'b', 'c' };
        testList.assign_range(AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testList, ::testing::ElementsAre('d', 'e', 'f'));
    }

    TEST_F(ListContainers, AssignRange_Succeeds)
    {
        ListAssignRangeSucceeds<AZStd::list>();
        ListAssignRangeSucceeds<AZStd::forward_list>();
    }

    TEST_F(ListContainers, List_InsertRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "abc";
        AZStd::list testList{ 'd', 'e', 'f' };
        testList.insert_range(testList.begin(), AZStd::vector<char>{testView.begin(), testView.end()});
        testList.insert_range(testList.end(), testView | AZStd::views::transform([](const char elem) -> char { return elem + 6; }));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(ListContainers, ForwardList_InsertAfterRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "abc";
        AZStd::forward_list testList{ 'd', 'e', 'f' };
        testList.insert_range_after(testList.before_begin(), AZStd::vector<char>{testView.begin(), testView.end()});
        testList.insert_range_after(testList.before_end(), testView | AZStd::views::transform([](const char elem) -> char { return elem + 6; }));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    template<template<class...> class ListTemplate>
    static void ListAppendRangeSucceeds()
    {
        constexpr AZStd::string_view testView = "def";
        ListTemplate testList{ 'a', 'b', 'c' };
        testList.append_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testList.append_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + 3; }));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(ListContainers, AppendRange_Succeeds)
    {
        ListAppendRangeSucceeds<AZStd::list>();
        ListAppendRangeSucceeds<AZStd::forward_list>();
    }

    template<template<class...> class ListTemplate>
    static void ListPrependRangeSucceeds()
    {
        constexpr AZStd::string_view testView = "def";
        ListTemplate testList{ 'g', 'h', 'i' };
        testList.prepend_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testList.prepend_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + -3; }));
        EXPECT_THAT(testList, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(ListContainers, PrependRange_Succeeds)
    {
        ListPrependRangeSucceeds<AZStd::list>();
        ListPrependRangeSucceeds<AZStd::forward_list>();
    }
}

#undef AZ_TEST_VALIDATE_EMPTY_LIST
#undef AZ_TEST_VALIDATE_LIST

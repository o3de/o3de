/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/allocator_ref.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using namespace AZStd;
using namespace UnitTestInternal;

#define AZ_TEST_VALIDATE_EMPTY_LIST(_List)        \
    AZ_TEST_ASSERT(_List.validate());             \
    AZ_TEST_ASSERT(_List.size() == 0);            \
    AZ_TEST_ASSERT(_List.begin() == _List.end()); \
    AZ_TEST_ASSERT(_List.empty());

#define AZ_TEST_VALIDATE_LIST(_List, _NumElements)                       \
    AZ_TEST_ASSERT(_List.validate());                                    \
    AZ_TEST_ASSERT(_List.size() == _NumElements);                        \
    AZ_TEST_ASSERT((_NumElements > 0) ? !_List.empty() : _List.empty()); \
    AZ_TEST_ASSERT((_NumElements > 0) ? _List.begin() != _List.end() : _List.begin() == _List.end());

namespace UnitTest
{
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
        list<int> intList({ 1, 2, 3, 4, 5, 6 });
        EXPECT_EQ(6, intList.size());
    }

    TEST_F(ListContainers, ListCtorAssign)
    {
        list<int>   int_list;
        list<int>   int_list1;
        list<int>   int_list2;
        list<int>   int_list3;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);

        // 10 int elements, value 33
        int_list1 = list<int>(10, 33);
        AZ_TEST_VALIDATE_LIST(int_list1, 10);
        for (list<int>::iterator iter = int_list1.begin(); iter != int_list1.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 33);
        }

        // copy list 1 using, first,last,allocator.
        int_list2 = list<int>(int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list2, 10);
        for (list<int>::iterator iter = int_list2.begin(); iter != int_list2.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 33);
        }

        // copy construct
        int_list3 = list<int>(int_list1);
        AZ_TEST_ASSERT(int_list1 == int_list3);

        // assign
        int_list = int_list1;
        AZ_TEST_ASSERT(int_list == int_list1);

        int_list2.push_back(60);
        int_list = int_list2;
        AZ_TEST_ASSERT(int_list == int_list2);

        int_list2.pop_back();
        int_list2.pop_back();
        int_list = int_list2;
        AZ_TEST_ASSERT(int_list == int_list2);

        // assign with iterators (at the moment this uses the function operator= calls)
        int_list2.assign(int_list1.begin(), int_list1.end());
        AZ_TEST_ASSERT(int_list2 == int_list1);

        // assign a fixed value
        int_list.assign(5, 55);
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        for (list<int>::iterator iter = int_list.begin(); iter != int_list.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 55);
        }
    }

    TEST_F(ListContainers, ListResizeInsertEraseClear)
    {
        list<int>   int_list;
        list<int>   int_list1;
        list<int>   int_list2;
        list<int>   int_list3;
        list<int>::iterator list_it;

        int_list.assign(5, 55);
        int_list1 = list<int>(10, 33);

        // resize to bigger
        int_list.resize(6, 43);
        AZ_TEST_VALIDATE_LIST(int_list, 6);
        AZ_TEST_ASSERT(int_list.back() == 43);
        AZ_TEST_ASSERT(*prev(int_list.end(), 2) == 55);
        AZ_TEST_ASSERT(*int_list.begin() == *prev(int_list.rend()));

        // resize to smaller
        int_list.resize(3);
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        AZ_TEST_ASSERT(int_list.back() == 55);

        // insert
        list_it = int_list.insert(int_list.begin(), 44);
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        AZ_TEST_ASSERT(int_list.front() == 44);
        AZ_TEST_ASSERT(*list_it == 44);

        list_it = int_list.insert(int_list.end(), 66);
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        AZ_TEST_ASSERT(int_list.back() == 66);
        AZ_TEST_ASSERT(*list_it == 66);

        list_it = int_list.insert(int_list.begin(), 2, 11);
        AZ_TEST_VALIDATE_LIST(int_list, 7);
        AZ_TEST_ASSERT(int_list.front() == 11);
        AZ_TEST_ASSERT(*next(int_list.begin()) == 11);
        AZ_TEST_ASSERT(*list_it == 11);
        AZ_TEST_ASSERT(*next(list_it) == 11);

        list_it = int_list.insert(int_list.end(), 2, 22);
        AZ_TEST_VALIDATE_LIST(int_list, 9);
        AZ_TEST_ASSERT(int_list.back() == 22);
        AZ_TEST_ASSERT(*prev(int_list.end(), 2) == 22);
        AZ_TEST_ASSERT(*prev(list_it) == 66); // 66 was the last element added at int_list.end()
        AZ_TEST_ASSERT(*list_it == 22);
        AZ_TEST_ASSERT(*next(list_it) == 22);

        list_it = int_list.insert(int_list.end(), int_list1.begin(), int_list1.end());
        AZ_TEST_VALIDATE_LIST(int_list, 9 + int_list1.size());
        AZ_TEST_ASSERT(int_list.back() == 33);

        // Ensure that parallel iterating over int_list1 and int_list
        // from the returned iterator result in the same elements.
        list<int>::iterator list1_it = int_list1.begin();
        for (; list1_it != int_list1.end(); ++list_it, ++list1_it)
        {
            AZ_TEST_ASSERT(list_it != int_list.end());
            AZ_TEST_ASSERT(list1_it != int_list1.end());
            AZ_TEST_ASSERT(*list_it == *list_it);
        }

        // erase
        int_list.assign(2, 10);
        int_list.push_back(20);
        int_list.erase(--int_list.end());
        AZ_TEST_VALIDATE_LIST(int_list, 2);
        AZ_TEST_ASSERT(int_list.back() == 10);

        int_list.insert(int_list.end(), 3, 44);
        int_list.erase(prev(int_list.end(), 2), int_list.end());
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        AZ_TEST_ASSERT(int_list.back() == 44);

        // clear
        int_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list);
    }

    TEST_F(ListContainers, SwapSplice)
    {
        list<int>   int_list;
        list<int>   int_list1;
        list<int>   int_list2;

        int_list2 = list<int>(10, 33);

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
        AZ_TEST_ASSERT(int_list.front() == 33);
        AZ_TEST_ASSERT(int_list2.front() == 55);

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_list.splice(int_list.end(), int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 15);
        AZ_TEST_ASSERT(int_list.front() == 33);
        AZ_TEST_ASSERT(int_list.back() == 55);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_list2.push_back(101);
        int_list.splice(int_list.begin(), int_list2, int_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 16);
        AZ_TEST_ASSERT(int_list.front() == 101);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_list2.assign(5, 201);
        int_list.splice(int_list.end(), int_list2, ++int_list2.begin(), int_list2.end());
        AZ_TEST_VALIDATE_LIST(int_list2, 1);
        AZ_TEST_VALIDATE_LIST(int_list, 20);
        AZ_TEST_ASSERT(int_list.back() == 201);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        int_list2.push_back(301);
        int_list.splice(int_list.end(), int_list2, int_list2.begin(), int_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list2);
        AZ_TEST_VALIDATE_LIST(int_list, 22);
        AZ_TEST_ASSERT(int_list.back() == 301);
    }

    TEST_F(ListContainers, RemoveUniqueSort)
    {
        list<int>   int_list;

        // remove
        int_list.assign(5, 101);
        int_list.push_back(201);
        int_list.push_back(301);
        int_list.push_back(401);
        int_list.remove(101);
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        AZ_TEST_ASSERT(int_list.front() == 201);
        AZ_TEST_ASSERT(int_list.back() == 401);

        int_list.remove_if(RemoveLessThan401());
        AZ_TEST_VALIDATE_LIST(int_list, 1);
        AZ_TEST_ASSERT(int_list.back() == 401);

        // unique
        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(201);
        int_list.push_back(301);
        int_list.unique();
        AZ_TEST_VALIDATE_LIST(int_list, 3);
        AZ_TEST_ASSERT(int_list.front() == 101);
        AZ_TEST_ASSERT(int_list.back() == 301);

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(201);
        int_list.push_back(401);
        int_list.push_back(401);
        int_list.push_back(501);
        int_list.unique(UniqueForLessThan401());
        AZ_TEST_VALIDATE_LIST(int_list, 5);
        AZ_TEST_ASSERT(int_list.front() == 101);
        AZ_TEST_ASSERT(int_list.back() == 501);

        // sort
        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort();
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        for (list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter <= *next(iter));
        }

        int_list.assign(2, 101);
        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_list, 4);
        for (list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter >= *next(iter));
        }
    }

    TEST_F(ListContainers, ReverseMerge)
    {
        list<int>   int_list;
        list<int>   int_list1;
        list<int>   int_list2;
        list<int>   int_list3;

        int_list.push_back(201);
        int_list.push_back(1);
        int_list.sort(AZStd::greater<int>());

        // reverse
        int_list.reverse();
        for (list<int>::iterator iter = int_list.begin(); iter != --int_list.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter <= *next(iter));
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
        for (list<int>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter < *next(iter));
        }

        int_list.reverse();
        int_list1.reverse();

        int_list2 = int_list;
        int_list3 = int_list1;
        int_list2.merge(int_list3, AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_list2, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list3);
        for (list<int>::iterator iter = int_list2.begin(); iter != --int_list2.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter > *next(iter));
        }
    }

    TEST_F(ListContainers, Extensions)
    {
        list<int>   int_list;
        // Extensions.
        int_list.clear();

        // Push_back()
        int_list.push_back();
        AZ_TEST_VALIDATE_LIST(int_list, 1);
        int_list.front() = 100;

        // Push_front()
        int_list.push_front();
        AZ_TEST_VALIDATE_LIST(int_list, 2);
        AZ_TEST_ASSERT(int_list.back() == 100);

        // Insert without value to copy from.
        int_list.insert(int_list.begin());
        AZ_TEST_VALIDATE_LIST(int_list, 3);

        // default int alignment
        AZ_TEST_ASSERT(((AZStd::size_t)&int_list.front() % 4) == 0); // default int alignment

        // make sure every allocation is aligned.
        list<MyClass> aligned_list(5, MyClass(99));
        AZ_TEST_ASSERT(((AZStd::size_t)&aligned_list.front() & (alignment_of<MyClass>::value - 1)) == 0);
    }

    TEST_F(ListContainers, StaticBufferAllocator)
    {
        /////////////////////////////////////////////////////////////////////////
        // Test swap, splice, merge, etc. of containers with different allocators.
        // list operations with container with the same allocator.
        typedef static_buffer_allocator<16*1024, 1> static_buffer_16KB_type;
        static_buffer_16KB_type myMemoryManager1;
        static_buffer_16KB_type myMemoryManager2;
        typedef allocator_ref<static_buffer_16KB_type> static_allocator_ref_type;
        static_allocator_ref_type allocator1(myMemoryManager1, "Mystack allocator 1");
        static_allocator_ref_type allocator2(myMemoryManager2, "Mystack allocator 2");

        typedef list<MyClass, static_allocator_ref_type> stack_myclass_list_type;
        stack_myclass_list_type int_list10(allocator1);
        stack_myclass_list_type int_list20(allocator2);

        int_list20.assign(10, MyClass(33));
        AZ_TEST_VALIDATE_LIST(int_list20, 10);
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 10 * sizeof(stack_myclass_list_type::node_type));

        int_list20.leak_and_reset();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        myMemoryManager2.reset(); // free all memory

        // set allocator
        int_list20.assign(20, MyClass(22));
        int_list20.set_allocator(allocator1);
        AZ_TEST_VALIDATE_LIST(int_list20, 20);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 20 * sizeof(stack_myclass_list_type::node_type));
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 20 * sizeof(stack_myclass_list_type::node_type));
        int_list20.leak_and_reset();
        int_list20.set_allocator(allocator2);
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        int_list20.assign(10, MyClass(11));

        // swap
        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 10);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 10 * sizeof(stack_myclass_list_type::node_type));
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 10 * sizeof(stack_myclass_list_type::node_type));

        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list10);
        AZ_TEST_VALIDATE_LIST(int_list20, 10);

        int_list10.assign(5, 55);
        int_list10.swap(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 10);
        AZ_TEST_VALIDATE_LIST(int_list20, 5);
        AZ_TEST_ASSERT(int_list10.front() == MyClass(11));
        AZ_TEST_ASSERT(int_list20.front() == MyClass(55));

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_list10.splice(int_list10.end(), int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 15);
        AZ_TEST_ASSERT(int_list10.front() == MyClass(11));
        AZ_TEST_ASSERT(int_list10.back() == MyClass(55));
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_list20.push_back(MyClass(101));
        int_list10.splice(int_list10.begin(), int_list20, int_list20.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list20);
        AZ_TEST_VALIDATE_LIST(int_list10, 16);
        AZ_TEST_ASSERT(int_list10.front() == MyClass(101));

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_list20.assign(5, MyClass(201));
        int_list10.splice(int_list10.end(), int_list20, ++int_list20.begin(), int_list20.end());
        AZ_TEST_VALIDATE_LIST(int_list20, 1);
        AZ_TEST_VALIDATE_LIST(int_list10, 20);
        AZ_TEST_ASSERT(int_list10.back() == MyClass(201));

        int_list10.leak_and_reset();
        int_list20.leak_and_reset();
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        // merge
        stack_myclass_list_type int_list30(allocator1);
        stack_myclass_list_type int_list40(allocator2);
        int_list10.push_back(MyClass(1)); // 2 sorted lists for merge
        int_list10.push_back(MyClass(10));
        int_list10.push_back(MyClass(50));
        int_list10.push_back(MyClass(200));
        int_list20.push_back(MyClass(2));
        int_list20.push_back(MyClass(8));
        int_list20.push_back(MyClass(60));
        int_list20.push_back(MyClass(180));

        int_list30 = int_list10;
        int_list40 = int_list20;
        int_list30.merge(int_list40);
        AZ_TEST_VALIDATE_LIST(int_list30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list40);
        for (stack_myclass_list_type::iterator iter = int_list30.begin(); iter != --int_list30.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter < *next(iter));
        }

        int_list10.reverse();
        int_list20.reverse();

        int_list30 = int_list10;
        int_list40 = int_list20;
        int_list30.merge(int_list40, AZStd::greater<MyClass>());
        AZ_TEST_VALIDATE_LIST(int_list30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_list40);
        for (stack_myclass_list_type::iterator iter = int_list30.begin(); iter != --int_list30.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter > *next(iter));
        }
    }

    TEST_F(ListContainers, UniquePtr)
    {
        list<unique_ptr<MyNoCopyClass>> nocopy_list;
        nocopy_list.emplace_back(new MyNoCopyClass(1, true, 3.0f));
        nocopy_list.emplace_front(new MyNoCopyClass(2, true, 4.0f));
        nocopy_list.emplace(nocopy_list.end(), new MyNoCopyClass(3, true, 5.0f));

        nocopy_list.insert(nocopy_list.end(), AZStd::unique_ptr<MyNoCopyClass>(new MyNoCopyClass(4, true, 6.0f)));

        for (const auto& ptr : nocopy_list)
        {
            AZ_TEST_ASSERT(ptr->m_bool);
        }
    }

    TEST_F(ListContainers, ForwardListAssign)
    {
        forward_list<int> int_slist;
        forward_list<int> int_slist1;
        forward_list<int> int_slist2;
        forward_list<int> int_slist3;
        forward_list<int> int_slist4;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist);

        // 10 int elements, value 33
        int_slist1 = forward_list<int>(10, 33);
        AZ_TEST_VALIDATE_LIST(int_slist1, 10);
        for (forward_list<int>::iterator iter = int_slist1.begin(); iter != int_slist1.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 33);
        }

        // copy list 1 using, first,last,allocator.
        int_slist2 = forward_list<int>(int_slist1.begin(), int_slist1.end());
        AZ_TEST_VALIDATE_LIST(int_slist2, 10);
        for (forward_list<int>::iterator iter = int_slist2.begin(); iter != int_slist2.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 33);
        }

        // copy construct
        int_slist3 = forward_list<int>(int_slist1);
        AZ_TEST_ASSERT(int_slist1 == int_slist3);

        // initializer_list construct
        int_slist4 = { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33 };
        AZ_TEST_ASSERT(int_slist1 == int_slist4);

        // assign
        int_slist = int_slist1;
        AZ_TEST_ASSERT(int_slist == int_slist1);

        int_slist2.push_back(60);
        int_slist = int_slist2;
        AZ_TEST_ASSERT(int_slist == int_slist2);

        //int_slist2.pop_back();
        //int_slist2.pop_back();
        //int_slist = int_slist2;
        //AZ_TEST_ASSERT(int_slist==int_slist2);

        // assign with iterators (at the moment this uses the function operator= calls)
        int_slist2.assign(int_slist1.begin(), int_slist1.end());
        AZ_TEST_ASSERT(int_slist2 == int_slist1);

        // assign a fixed value
        int_slist.assign(5, 55);
        AZ_TEST_VALIDATE_LIST(int_slist, 5);
        for (forward_list<int>::iterator iter = int_slist.begin(); iter != int_slist.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 55);
        }
    }

    TEST_F(ListContainers, ForwardListInsertEraseClear)
    {
        forward_list<int> int_slist;
        forward_list<int> int_slist1;

        int_slist.assign(5, 55);
        int_slist1 = forward_list<int>(10, 33);
        
        // resize to bigger
        int_slist.resize(6, 43);
        AZ_TEST_VALIDATE_LIST(int_slist, 6);
        AZ_TEST_ASSERT(int_slist.back() == 43);

        // resize to smaller
        int_slist.resize(3);
        AZ_TEST_VALIDATE_LIST(int_slist, 3);
        AZ_TEST_ASSERT(int_slist.back() == 55);

        // insert
        int_slist.insert(int_slist.begin(), 44);
        AZ_TEST_VALIDATE_LIST(int_slist, 4);
        AZ_TEST_ASSERT(int_slist.front() == 44);

        int_slist.insert(int_slist.end(), 66);
        AZ_TEST_VALIDATE_LIST(int_slist, 5);
        AZ_TEST_ASSERT(int_slist.back() == 66);

        int_slist.insert(int_slist.begin(), 2, 11);
        AZ_TEST_VALIDATE_LIST(int_slist, 7);
        AZ_TEST_ASSERT(int_slist.front() == 11);
        AZ_TEST_ASSERT(*next(int_slist.begin()) == 11);

        int_slist.insert(int_slist.end(), 2, 22);
        AZ_TEST_VALIDATE_LIST(int_slist, 9);
        AZ_TEST_ASSERT(int_slist.back() == 22);
        AZ_TEST_ASSERT(*int_slist.previous(int_slist.last()) == 22);

        int_slist.insert(int_slist.end(), int_slist1.begin(), int_slist1.end());
        AZ_TEST_VALIDATE_LIST(int_slist, 9 + int_slist1.size());
        AZ_TEST_ASSERT(int_slist.back() == 33);

        // erase
        int_slist.assign(2, 10);
        int_slist.push_back(20);
        int_slist.erase(int_slist.last());
        AZ_TEST_VALIDATE_LIST(int_slist, 2);
        AZ_TEST_ASSERT(int_slist.back() == 10);

        int_slist.insert(int_slist.end(), 3, 44);
        int_slist.erase(int_slist.previous(int_slist.last()), int_slist.end());
        AZ_TEST_VALIDATE_LIST(int_slist, 3);
        AZ_TEST_ASSERT(int_slist.back() == 44);

        // clear
        int_slist.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist);
    }

    TEST_F(ListContainers, ForwardListSwapSplice)
    {
        forward_list<int> int_slist;
        forward_list<int> int_slist1;
        forward_list<int> int_slist2;

        int_slist2 = forward_list<int>(10, 33);

        // list operations with container with the same allocator.
        // swap
        int_slist.swap(int_slist2);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist2);
        AZ_TEST_VALIDATE_LIST(int_slist, 10);

        int_slist.swap(int_slist2);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist);
        AZ_TEST_VALIDATE_LIST(int_slist2, 10);

        int_slist.assign(5, 55);
        int_slist.swap(int_slist2);
        AZ_TEST_VALIDATE_LIST(int_slist, 10);
        AZ_TEST_VALIDATE_LIST(int_slist2, 5);
        AZ_TEST_ASSERT(int_slist.front() == 33);
        AZ_TEST_ASSERT(int_slist2.front() == 55);

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_slist.splice(int_slist.end(), int_slist2);
        AZ_TEST_VALIDATE_LIST(int_slist, 15);
        AZ_TEST_ASSERT(int_slist.front() == 33);
        AZ_TEST_ASSERT(int_slist.back() == 55);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist2);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_slist2.push_back(101);
        int_slist.splice(int_slist.begin(), int_slist2, int_slist2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist2);
        AZ_TEST_VALIDATE_LIST(int_slist, 16);
        AZ_TEST_ASSERT(int_slist.front() == 101);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_slist2.assign(5, 201);
        int_slist.splice(int_slist.end(), int_slist2, ++int_slist2.begin(), int_slist2.end());
        AZ_TEST_VALIDATE_LIST(int_slist2, 1);
        AZ_TEST_VALIDATE_LIST(int_slist, 20);
        AZ_TEST_ASSERT(int_slist.back() == 201);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        int_slist2.push_back(301);
        int_slist.splice(int_slist.end(), int_slist2, int_slist2.begin(), int_slist2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist2);
        AZ_TEST_VALIDATE_LIST(int_slist, 22);
        AZ_TEST_ASSERT(int_slist.back() == 301);
    }

    TEST_F(ListContainers, ForwardListRemoveUniqueSort)
    {
        forward_list<int> int_slist;
        
        // remove
        int_slist.assign(5, 101);
        int_slist.push_back(201);
        int_slist.push_back(301);
        int_slist.push_back(401);
        int_slist.remove(101);
        AZ_TEST_VALIDATE_LIST(int_slist, 3);
        AZ_TEST_ASSERT(int_slist.front() == 201);
        AZ_TEST_ASSERT(int_slist.back() == 401);

        int_slist.remove_if(RemoveLessThan401());
        AZ_TEST_VALIDATE_LIST(int_slist, 1);
        AZ_TEST_ASSERT(int_slist.back() == 401);

        // unique
        int_slist.assign(2, 101);
        int_slist.push_back(201);
        int_slist.push_back(201);
        int_slist.push_back(301);
        int_slist.unique();
        AZ_TEST_VALIDATE_LIST(int_slist, 3);
        AZ_TEST_ASSERT(int_slist.front() == 101);
        AZ_TEST_ASSERT(int_slist.back() == 301);

        int_slist.assign(2, 101);
        int_slist.push_back(201);
        int_slist.push_back(201);
        int_slist.push_back(401);
        int_slist.push_back(401);
        int_slist.push_back(501);
        int_slist.unique(UniqueForLessThan401());
        AZ_TEST_VALIDATE_LIST(int_slist, 5);
        AZ_TEST_ASSERT(int_slist.front() == 101);
        AZ_TEST_ASSERT(int_slist.back() == 501);

        // sort
        int_slist.assign(2, 101);
        int_slist.push_back(201);
        int_slist.push_back(1);
        int_slist.sort();
        AZ_TEST_VALIDATE_LIST(int_slist, 4);
        for (forward_list<int>::iterator iter = int_slist.begin(); iter != int_slist.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter <= *next(iter));
        }

        int_slist.assign(2, 101);
        int_slist.push_back(201);
        int_slist.push_back(1);
        int_slist.sort(AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_slist, 4);
        for (forward_list<int>::iterator iter = int_slist.begin(); iter != int_slist.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter >= *next(iter));
        }
    }

    TEST_F(ListContainers, ForwardListReverseMerge)
    {
        forward_list<int> int_slist;
        forward_list<int> int_slist1;
        forward_list<int> int_slist2;
        forward_list<int> int_slist3;

        int_slist.assign(2, 101);
        int_slist.push_back(201);
        int_slist.push_back(1);
        int_slist.sort(AZStd::greater<int>());

        // reverse
        int_slist.reverse();
        for (forward_list<int>::iterator iter = int_slist.begin(); iter != int_slist.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter <= *next(iter));
        }

        // merge
        int_slist.clear();
        int_slist1.clear();
        int_slist.push_back(1);  // 2 sorted lists for merge
        int_slist.push_back(10);
        int_slist.push_back(50);
        int_slist.push_back(200);
        int_slist1.push_back(2);
        int_slist1.push_back(8);
        int_slist1.push_back(60);
        int_slist1.push_back(180);

        int_slist2 = int_slist;
        int_slist3 = int_slist1;
        int_slist2.merge(int_slist3);
        AZ_TEST_VALIDATE_LIST(int_slist2, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist3);
        for (forward_list<int>::iterator iter = int_slist2.begin(); iter != int_slist2.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter < *next(iter));
        }

        int_slist.reverse();
        int_slist1.reverse();

        int_slist2 = int_slist;
        int_slist3 = int_slist1;
        int_slist2.merge(int_slist3, AZStd::greater<int>());
        AZ_TEST_VALIDATE_LIST(int_slist2, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist3);
        for (forward_list<int>::iterator iter = int_slist2.begin(); iter != int_slist2.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter > *next(iter));
        }
    }

    TEST_F(ListContainers, ForwardListExtensions)
    {
        forward_list<int> int_slist;

        // Extensions.
        int_slist.clear();

        // Push_back()
        int_slist.push_back();
        AZ_TEST_VALIDATE_LIST(int_slist, 1);
        int_slist.front() = 100;

        // Push_front()
        int_slist.push_front();
        AZ_TEST_VALIDATE_LIST(int_slist, 2);
        AZ_TEST_ASSERT(int_slist.back() == 100);

        // Insert without value to copy from.
        int_slist.insert(int_slist.begin());
        AZ_TEST_VALIDATE_LIST(int_slist, 3);

        // default int alignment
        AZ_TEST_ASSERT(((AZStd::size_t)&int_slist.front() % 4) == 0); // default int alignment

        // make sure every allocation is aligned.
        forward_list<MyClass> aligned_list(5, MyClass(99));
        AZ_TEST_ASSERT(((AZStd::size_t)&aligned_list.front() & (alignment_of<MyClass>::value - 1)) == 0);
    }

    TEST_F(ListContainers, ForwardListStaticBuffer)
    {
        // Test swap, splice, merge, etc. of containers with different allocators.
        // list operations with container with the same allocator.
        typedef static_buffer_allocator<16*1024, 1> static_buffer_16KB_type;
        static_buffer_16KB_type myMemoryManager1;
        static_buffer_16KB_type myMemoryManager2;
        typedef allocator_ref<static_buffer_16KB_type> static_allocator_ref_type;
        static_allocator_ref_type allocator1(myMemoryManager1, "Mystack allocator 1");
        static_allocator_ref_type allocator2(myMemoryManager2, "Mystack allocator 2");

        typedef forward_list<MyClass, static_allocator_ref_type> stack_myclass_slist_type;
        stack_myclass_slist_type   int_slist10(allocator1);
        stack_myclass_slist_type   int_slist20(allocator2);

        int_slist20.assign(10, MyClass(33));
        AZ_TEST_VALIDATE_LIST(int_slist20, 10);
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 10 * sizeof(stack_myclass_slist_type::node_type));

        int_slist20.leak_and_reset();
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist20);
        myMemoryManager2.reset();  // free all memory

        // set allocator
        int_slist20.assign(20, MyClass(22));
        int_slist20.set_allocator(allocator1);
        AZ_TEST_VALIDATE_LIST(int_slist20, 20);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 20 * sizeof(stack_myclass_slist_type::node_type));
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 20 * sizeof(stack_myclass_slist_type::node_type));
        int_slist20.leak_and_reset();
        int_slist20.set_allocator(allocator2);
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        int_slist20.assign(10, MyClass(11));

        // swap
        int_slist10.swap(int_slist20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist20);
        AZ_TEST_VALIDATE_LIST(int_slist10, 10);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 10 * sizeof(stack_myclass_slist_type::node_type));
        AZ_TEST_ASSERT(myMemoryManager2.get_allocated_size() >= 10 * sizeof(stack_myclass_slist_type::node_type));

        int_slist10.swap(int_slist20);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist10);
        AZ_TEST_VALIDATE_LIST(int_slist20, 10);

        int_slist10.assign(5, 55);
        int_slist10.swap(int_slist20);
        AZ_TEST_VALIDATE_LIST(int_slist10, 10);
        AZ_TEST_VALIDATE_LIST(int_slist20, 5);
        AZ_TEST_ASSERT(int_slist10.front() == MyClass(11));
        AZ_TEST_ASSERT(int_slist20.front() == MyClass(55));

        // splice
        // splice(iterator splicePos, this_type& rhs)
        int_slist10.splice(int_slist10.end(), int_slist20);
        AZ_TEST_VALIDATE_LIST(int_slist10, 15);
        AZ_TEST_ASSERT(int_slist10.front() == MyClass(11));
        AZ_TEST_ASSERT(int_slist10.back() == MyClass(55));
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist20);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        int_slist20.push_back(MyClass(101));
        int_slist10.splice(int_slist10.begin(), int_slist20, int_slist20.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist20);
        AZ_TEST_VALIDATE_LIST(int_slist10, 16);
        AZ_TEST_ASSERT(int_slist10.front() == MyClass(101));

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        int_slist20.assign(5, MyClass(201));
        int_slist10.splice(int_slist10.end(), int_slist20, ++int_slist20.begin(), int_slist20.end());
        AZ_TEST_VALIDATE_LIST(int_slist20, 1);
        AZ_TEST_VALIDATE_LIST(int_slist10, 20);
        AZ_TEST_ASSERT(int_slist10.back() == MyClass(201));

        int_slist10.leak_and_reset();
        int_slist20.leak_and_reset();
        myMemoryManager1.reset();
        myMemoryManager2.reset();

        // merge
        stack_myclass_slist_type   int_slist30(allocator1);
        stack_myclass_slist_type   int_slist40(allocator2);
        int_slist10.push_back(MyClass(1));  // 2 sorted lists for merge
        int_slist10.push_back(MyClass(10));
        int_slist10.push_back(MyClass(50));
        int_slist10.push_back(MyClass(200));
        int_slist20.push_back(MyClass(2));
        int_slist20.push_back(MyClass(8));
        int_slist20.push_back(MyClass(60));
        int_slist20.push_back(MyClass(180));

        int_slist30 = int_slist10;
        int_slist40 = int_slist20;
        int_slist30.merge(int_slist40);
        AZ_TEST_VALIDATE_LIST(int_slist30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist40);
        for (stack_myclass_slist_type::iterator iter = int_slist30.begin(); iter != int_slist30.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter < *next(iter));
        }

        int_slist10.reverse();
        int_slist20.reverse();

        int_slist30 = int_slist10;
        int_slist40 = int_slist20;
        int_slist30.merge(int_slist40, AZStd::greater<MyClass>());
        AZ_TEST_VALIDATE_LIST(int_slist30, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(int_slist40);
        for (stack_myclass_slist_type::iterator iter = int_slist30.begin(); iter != int_slist30.last(); ++iter)
        {
            AZ_TEST_ASSERT(*iter > *next(iter));
        }
    }
}

#undef AZ_TEST_VALIDATE_EMPTY_LIST
#undef AZ_TEST_VALIDATE_LIST

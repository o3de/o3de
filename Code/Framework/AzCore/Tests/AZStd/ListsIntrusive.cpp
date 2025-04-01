/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/functional.h>

#include <AzCore/std/containers/array.h>

#define AZ_TEST_VALIDATE_EMPTY_LIST(_List)        \
    EXPECT_TRUE(_List.validate());             \
    EXPECT_EQ(0, _List.size());            \
    EXPECT_TRUE(_List.begin() == _List.end()); \
    EXPECT_TRUE(_List.empty());


#define AZ_TEST_VALIDATE_LIST(_List, _NumElements)                                                    \
    EXPECT_TRUE(_List.validate());                                                                 \
    EXPECT_EQ(_NumElements, _List.size());                                                     \
    EXPECT_TRUE((_NumElements > 0) ? !_List.empty() : _List.empty());                              \
    EXPECT_TRUE((_NumElements > 0) ? _List.begin() != _List.end() : _List.begin() == _List.end()); \
    EXPECT_FALSE(_List.empty());

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    // IntrusiveListContainerTest-Begin

    // My intrusive list class.
    // We have 2 hooks in this class. One of each supported type. Base hook which we inherit from the intrusive_list_node node
    // and public member hook (m_listHook).
    struct MyListClass
        : public intrusive_list_node<MyListClass>
    {
    public:
        MyListClass(int data = 101)
            : m_data(data) {}

        intrusive_list_node<MyListClass>    m_listHook;     // Public member hook.

        int m_data; // This is left public only for testing purpose.
    };
    // Compare operators, used for sort, merge, etc.
    AZ_FORCE_INLINE bool operator==(const MyListClass& a, const MyListClass& b) { return a.m_data == b.m_data; }
    AZ_FORCE_INLINE bool operator!=(const MyListClass& a, const MyListClass& b) { return a.m_data != b.m_data; }
    AZ_FORCE_INLINE bool operator<(const MyListClass& a, const MyListClass& b)  { return a.m_data < b.m_data; }
    AZ_FORCE_INLINE bool operator>(const MyListClass& a, const MyListClass& b)  { return a.m_data > b.m_data; }

    class IntrusiveListContainers
        : public LeakDetectionFixture
    {
    public:
        template <class T>
        struct RemoveLessThan401
        {
            AZ_FORCE_INLINE bool operator()(const T& element) const { return element.m_data < 401; }
        };

        template <class T>
        struct UniqueForLessThan401
        {
            AZ_FORCE_INLINE bool operator()(const T& el1, const T& el2) const { return (el1.m_data == el2.m_data && el1.m_data < 401); }
        };

        template <class T, size_t S>
        void FillArray(array<T, S>& myclassArray)
        {
            for (int i = 0; i < (int)myclassArray.size(); ++i)
            {
                myclassArray[i].m_data = 101 + i * 100;
            }
        }
    };

    TEST_F(IntrusiveListContainers, IntrusiveListCtorAssign)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;
        typedef intrusive_list<MyListClass, list_member_hook<MyListClass, &MyListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);

        //
        myclass_base_list_type myclass_base_list21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list21, myclassArray.size());
        for (myclass_base_list_type::const_iterator iter = myclass_base_list21.begin(); iter != myclass_base_list21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        myclass_member_list_type myclass_member_list21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list21, myclassArray.size());
        for (myclass_member_list_type::const_iterator iter = myclass_member_list21.begin(); iter != myclass_member_list21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        // assign
        myclass_base_list21.clear();
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());

        myclass_member_list21.clear();
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
    }

    TEST_F(IntrusiveListContainers, IntrusiveListResizeInsertEraseClear)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;
        typedef intrusive_list<MyListClass, list_member_hook<MyListClass, &MyListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;

        // insert
        myclass_base_list.clear();
        myclass_member_list.clear();

        myclass_base_list.insert(myclass_base_list.begin(), *myclassArray.begin());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == myclassArray.begin()->m_data);

        myclass_member_list.insert(myclass_member_list.begin(), *myclassArray.begin());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == myclassArray.begin()->m_data);

        myclass_base_list.insert(myclass_base_list.begin(), next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == prev(myclassArray.end())->m_data);

        myclass_member_list.insert(myclass_member_list.begin(), next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == prev(myclassArray.end())->m_data);

        // erase
        myclass_base_list.erase(prev(myclass_base_list.end()));
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        myclass_base_list.erase(myclass_base_list.begin());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 2);

        myclass_member_list.erase(prev(myclass_member_list.end()));
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        myclass_member_list.erase(myclass_member_list.begin());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 2);

        myclass_base_list.erase(next(myclass_base_list.begin()), myclass_base_list.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);
        myclass_member_list.erase(next(myclass_member_list.begin()), myclass_member_list.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);

        // clear
        myclass_base_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        myclass_member_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);
    }

    TEST_F(IntrusiveListContainers, IntrusiveListSwapSplace)
    {
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;
        typedef intrusive_list<MyListClass, list_member_hook<MyListClass, &MyListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;
        myclass_base_list_type      myclass_base_list2;
        myclass_member_list_type    myclass_member_list;
        myclass_member_list_type    myclass_member_list2;

        // swap
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());

        myclass_base_list2.swap(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, myclassArray.size());

        myclass_member_list2.swap(myclass_member_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, myclassArray.size());

        myclass_base_list2.swap(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());

        myclass_member_list2.swap(myclass_member_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());

        myclass_base_list.pop_back();
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        myclass_member_list.pop_back();
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);

        myclass_base_list2.push_back(myclassArray.back());
        myclass_member_list2.push_back(myclassArray.back());

        myclass_base_list.swap(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, myclassArray.size() - 1);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);

        myclass_member_list.swap(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, myclassArray.size() - 1);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);

        // splice
        myclassArray.back().m_data = 10;

        // splice(iterator splicePos, this_type& rhs)
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 10);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 101);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 10);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 101);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);

        myclassArray.back().m_data = 20;
        myclass_base_list.pop_front();
        myclass_member_list.pop_front();
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);      // Just to confirm we poped the first element
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        myclass_base_list2.push_back(myclassArray.back());
        myclass_base_list.splice(myclass_base_list.begin(), myclass_base_list2, myclass_base_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 20);

        myclass_member_list2.push_back(myclassArray.back());
        myclass_member_list.splice(myclass_member_list.begin(), myclass_member_list2, myclass_member_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 20);

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();

        myclass_base_list.assign(myclassArray.begin(), next(next(myclassArray.begin())));
        myclass_member_list.assign(myclassArray.begin(), next(next(myclassArray.begin())));
        myclass_base_list2.assign(next(next(myclassArray.begin())), myclassArray.end());
        myclass_member_list2.assign(next(next(myclassArray.begin())), myclassArray.end());
        myclassArray.back().m_data = 201;

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2, next(myclass_base_list2.begin()), myclass_base_list2.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, 1);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 201);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2, next(myclass_member_list2.begin()), myclass_member_list2.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, 1);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 201);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        myclass_base_list2.back().m_data = 301;
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2, myclass_base_list2.begin(), myclass_base_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 301);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2, myclass_member_list2.begin(), myclass_member_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 301);

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();
    }

    TEST_F(IntrusiveListContainers, IntrusiveListRemoveUniqueSortReverse)
    {
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;
        typedef intrusive_list<MyListClass, list_member_hook<MyListClass, &MyListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;
        
        FillArray(myclassArray);

        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());

        // remove
        myclass_base_list.remove(MyListClass(101));
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 201);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 101 + ((int)myclassArray.size() - 1) * 100);

        myclass_base_list.remove_if(RemoveLessThan401<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 3);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 401);

        myclass_member_list.remove(MyListClass(101));
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 201);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 101 + ((int)myclassArray.size() - 1) * 100);

        myclass_member_list.remove_if(RemoveLessThan401<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 3);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 401);

        // unique
        array<MyListClass, 5> nonUniqueArray = {
            {101, 101, 201, 201, 301}
        };
        array<MyListClass, 7> nonUniqueArray2 = {
            {101, 101, 201, 201, 401, 401, 501}
        };
        myclass_base_list.assign(nonUniqueArray.begin(), nonUniqueArray.end());
        myclass_base_list.unique();
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 3);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 301);

        myclass_member_list.assign(nonUniqueArray.begin(), nonUniqueArray.end());
        myclass_member_list.unique();
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 3);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 301);

        myclass_base_list.assign(nonUniqueArray2.begin(), nonUniqueArray2.end());
        myclass_base_list.unique(UniqueForLessThan401<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 5);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 501);

        myclass_member_list.assign(nonUniqueArray2.begin(), nonUniqueArray2.end());
        myclass_member_list.unique(UniqueForLessThan401<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 5);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 501);

        // sort
        array<MyListClass, 4> toSortArray = {
            {101, 101, 201, 1}
        };
        myclass_base_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_base_list.sort();
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 4);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != --myclass_base_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_base_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_base_list.sort(AZStd::greater<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 4);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != --myclass_base_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data >= (*next(iter)).m_data);
        }

        myclass_member_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_member_list.sort();
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 4);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != --myclass_member_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_member_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_member_list.sort(AZStd::greater<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 4);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != --myclass_member_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data >= (*next(iter)).m_data);
        }
        // reverse
        myclass_base_list.reverse();
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != --myclass_base_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_member_list.reverse();
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != --myclass_member_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_base_list.clear();
        myclass_member_list.clear();
    }

    TEST_F(IntrusiveListContainers, IntrusiveListRemoveWhileIterating)
    {
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;

        FillArray(myclassArray);

        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());

        // remove
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_base_list_type::const_iterator it = myclass_base_list.begin();
        while (it != myclass_base_list.end())
        {
            myclass_base_list.erase(it++);
        }
        AZ_TEST_ASSERT(myclass_base_list.empty());

        // repopulate and remove in reverse order
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_base_list_type::const_reverse_iterator rit = myclass_base_list.crbegin();
        while (rit != myclass_base_list.crend())
        {
            myclass_base_list.erase(*(rit++));
        }
        AZ_TEST_ASSERT(myclass_base_list.empty());
    }

    TEST_F(IntrusiveListContainers, IntrusiveListMerge)
    {
        typedef intrusive_list<MyListClass, list_base_hook<MyListClass> > myclass_base_list_type;
        typedef intrusive_list<MyListClass, list_member_hook<MyListClass, &MyListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MyListClass, 20> myclassArray;
        myclass_base_list_type      myclass_base_list;
        myclass_base_list_type      myclass_base_list2;
        myclass_member_list_type    myclass_member_list;
        myclass_member_list_type    myclass_member_list2;
        // merge
        // 2 sorted lists for merge
        array<MyListClass, 4> toMergeArray = {
            {1, 10, 50, 200}
        };
        array<MyListClass, 4> toMergeArray2 = {
            {2, 8, 60, 180}
        };

        myclass_base_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_base_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        myclass_base_list.merge(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != --myclass_base_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data < (*next(iter)).m_data);
        }

        myclass_base_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_base_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        // reverse the sort order
        myclass_base_list.reverse();
        myclass_base_list2.reverse();

        myclass_base_list.merge(myclass_base_list2, AZStd::greater<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != --myclass_base_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data > (*next(iter)).m_data);
        }

        myclass_member_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_member_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        myclass_member_list.merge(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != --myclass_member_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data < (*next(iter)).m_data);
        }

        myclass_member_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_member_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        // reverse the sort order
        myclass_member_list.reverse();
        myclass_member_list2.reverse();

        myclass_member_list.merge(myclass_member_list2, AZStd::greater<MyListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != --myclass_member_list.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data > (*next(iter)).m_data);
        }

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();
    }
    

    
    // We have 2 hooks in this class. One of each supported type. Base hook which we inherit from the intrusive_slist_node node
    // and public member hook (m_listHook).
    struct MySListClass
        : public intrusive_slist_node<MySListClass>
    {
    public:
        MySListClass(int data = 101)
            : m_data(data) {}

        intrusive_slist_node<MySListClass>  m_listHook;     // Public member hook.

        int m_data; // This is left public only for testing purpose.
    };
    // Compare operators, used for sort, merge, etc.
    AZ_FORCE_INLINE bool operator==(const MySListClass& a, const MySListClass& b)   { return a.m_data == b.m_data; }
    AZ_FORCE_INLINE bool operator!=(const MySListClass& a, const MySListClass& b)   { return a.m_data != b.m_data; }
    AZ_FORCE_INLINE bool operator<(const MySListClass& a, const MySListClass& b)  { return a.m_data < b.m_data; }
    AZ_FORCE_INLINE bool operator>(const MySListClass& a, const MySListClass& b)  { return a.m_data > b.m_data; }

    TEST_F(IntrusiveListContainers, IntrusiveSListCtorAssign)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_slist<MySListClass, slist_base_hook<MySListClass> > myclass_base_list_type;
        typedef intrusive_slist<MySListClass, slist_member_hook<MySListClass, &MySListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySListClass, 20> myclassArray;

        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);

        //
        myclass_base_list_type myclass_base_list21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list21, myclassArray.size());
        for (myclass_base_list_type::const_iterator iter = myclass_base_list21.begin(); iter != myclass_base_list21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        myclass_member_list_type    myclass_member_list21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list21, myclassArray.size());
        for (myclass_member_list_type::const_iterator iter = myclass_member_list21.begin(); iter != myclass_member_list21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        // assign
        myclass_base_list21.clear();
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());

        myclass_member_list21.clear();
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
    }

    TEST_F(IntrusiveListContainers, IntrusiveSListResizeInsertEraseClear)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_slist<MySListClass, slist_base_hook<MySListClass> > myclass_base_list_type;
        typedef intrusive_slist<MySListClass, slist_member_hook<MySListClass, &MySListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySListClass, 20> myclassArray;

        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;

        // insert
        myclass_base_list.clear();
        myclass_member_list.clear();

        myclass_base_list.insert(myclass_base_list.begin(), *myclassArray.begin());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == myclassArray.begin()->m_data);

        myclass_member_list.insert(myclass_member_list.begin(), *myclassArray.begin());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == myclassArray.begin()->m_data);

        myclass_base_list.insert(myclass_base_list.begin(), next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == prev(myclassArray.end())->m_data);

        myclass_member_list.insert(myclass_member_list.begin(), next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == prev(myclassArray.end())->m_data);

        // erase
        myclass_base_list.erase(myclass_base_list.last());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        myclass_base_list.erase(myclass_base_list.begin());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 2);

        myclass_member_list.erase(myclass_member_list.last());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        myclass_member_list.erase(myclass_member_list.begin());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 2);

        myclass_base_list.erase(next(myclass_base_list.begin()), myclass_base_list.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);
        myclass_member_list.erase(next(myclass_member_list.begin()), myclass_member_list.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);

        // clear
        myclass_base_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        myclass_member_list.clear();
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);
    }

    TEST_F(IntrusiveListContainers, IntrusiveSListSwapSplice)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_slist<MySListClass, slist_base_hook<MySListClass> > myclass_base_list_type;
        typedef intrusive_slist<MySListClass, slist_member_hook<MySListClass, &MySListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySListClass, 20> myclassArray;

        myclass_base_list_type      myclass_base_list;
        myclass_base_list_type      myclass_base_list2;
        myclass_member_list_type    myclass_member_list;
        myclass_member_list_type    myclass_member_list2;

        // swap
        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());

        myclass_base_list2.swap(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list);
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, myclassArray.size());

        myclass_member_list2.swap(myclass_member_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list);
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, myclassArray.size());

        myclass_base_list2.swap(myclass_base_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());

        myclass_member_list2.swap(myclass_member_list);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());

        // pop the last element
        myclass_base_list.erase(myclass_base_list.last());
        myclass_member_list.erase(myclass_member_list.last());

        myclass_base_list2.push_back(myclassArray.back());
        myclass_member_list2.push_back(myclassArray.back());

        myclass_base_list.swap(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, myclassArray.size() - 1);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 1);

        myclass_member_list.swap(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, myclassArray.size() - 1);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 1);

        // splice
        myclassArray.back().m_data = 10;

        // splice(iterator splicePos, this_type& rhs)
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 10);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 101);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 10);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 101);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);

        myclassArray.back().m_data = 20;
        myclass_base_list.pop_front();
        myclass_member_list.pop_front();
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);      // Just to confirm we poped the first element
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);

        // splice(iterator splicePos, this_type& rhs, iterator first)
        myclass_base_list2.push_back(myclassArray.back());
        myclass_base_list.splice(myclass_base_list.begin(), myclass_base_list2, myclass_base_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 20);

        myclass_member_list2.push_back(myclassArray.back());
        myclass_member_list.splice(myclass_member_list.begin(), myclass_member_list2, myclass_member_list2.begin());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 20);

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();

        myclass_base_list.assign(myclassArray.begin(), next(next(myclassArray.begin())));
        myclass_member_list.assign(myclassArray.begin(), next(next(myclassArray.begin())));
        myclass_base_list2.assign(next(next(myclassArray.begin())), myclassArray.end());
        myclass_member_list2.assign(next(next(myclassArray.begin())), myclassArray.end());
        myclassArray.back().m_data = 201;

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2, next(myclass_base_list2.begin()), myclass_base_list2.end());
        AZ_TEST_VALIDATE_LIST(myclass_base_list2, 1);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 201);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2, next(myclass_member_list2.begin()), myclass_member_list2.end());
        AZ_TEST_VALIDATE_LIST(myclass_member_list2, 1);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 201);

        // splice(iterator splicePos, this_type& rhs, iterator first, iterator last) the whole vector optimization.
        myclass_base_list2.back().m_data = 301;
        myclass_base_list.splice(myclass_base_list.end(), myclass_base_list2, myclass_base_list2.begin(), myclass_base_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 301);

        myclass_member_list.splice(myclass_member_list.end(), myclass_member_list2, myclass_member_list2.begin(), myclass_member_list2.end());
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 301);

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();
    }

    TEST_F(IntrusiveListContainers, IntrusiveSListRemoveUniqueSortReverse)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_slist<MySListClass, slist_base_hook<MySListClass> > myclass_base_list_type;
        typedef intrusive_slist<MySListClass, slist_member_hook<MySListClass, &MySListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySListClass, 20> myclassArray;

        myclass_base_list_type      myclass_base_list;
        myclass_member_list_type    myclass_member_list;

        FillArray(myclassArray);

        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());
        myclass_member_list.assign(myclassArray.begin(), myclassArray.end());

        // remove
        myclass_base_list.remove(MySListClass(101));
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 201);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 101 + ((int)myclassArray.size() - 1) * 100);

        myclass_base_list.remove_if(RemoveLessThan401<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, myclassArray.size() - 3);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 401);

        myclass_member_list.remove(MySListClass(101));
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 1);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 201);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 101 + ((int)myclassArray.size() - 1) * 100);

        myclass_member_list.remove_if(RemoveLessThan401<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, myclassArray.size() - 3);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 401);

        // unique
        array<MySListClass, 5> nonUniqueArray = {
            {101, 101, 201, 201, 301}
        };
        array<MySListClass, 7> nonUniqueArray2 = {
            {101, 101, 201, 201, 401, 401, 501}
        };
        myclass_base_list.assign(nonUniqueArray.begin(), nonUniqueArray.end());
        myclass_base_list.unique();
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 3);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 301);

        myclass_member_list.assign(nonUniqueArray.begin(), nonUniqueArray.end());
        myclass_member_list.unique();
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 3);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 301);

        myclass_base_list.assign(nonUniqueArray2.begin(), nonUniqueArray2.end());
        myclass_base_list.unique(UniqueForLessThan401<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 5);
        AZ_TEST_ASSERT(myclass_base_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_base_list.back().m_data == 501);

        myclass_member_list.assign(nonUniqueArray2.begin(), nonUniqueArray2.end());
        myclass_member_list.unique(UniqueForLessThan401<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 5);
        AZ_TEST_ASSERT(myclass_member_list.front().m_data == 101);
        AZ_TEST_ASSERT(myclass_member_list.back().m_data == 501);

        // sort
        array<MySListClass, 4> toSortArray = {
            {101, 101, 201, 1}
        };
        myclass_base_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_base_list.sort();
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 4);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != myclass_base_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_base_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_base_list.sort(AZStd::greater<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 4);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != myclass_base_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data >= (*next(iter)).m_data);
        }

        myclass_member_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_member_list.sort();
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 4);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != myclass_member_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_member_list.assign(toSortArray.begin(), toSortArray.end());
        myclass_member_list.sort(AZStd::greater<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 4);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != myclass_member_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data >= (*next(iter)).m_data);
        }

        // reverse
        myclass_base_list.reverse();
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != myclass_base_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_member_list.reverse();
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != myclass_member_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data <= (*next(iter)).m_data);
        }

        myclass_base_list.clear();
        myclass_member_list.clear();
    }

    TEST_F(IntrusiveListContainers, IntrusiveSListMerge)
    {
        // Create 2 list types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_slist<MySListClass, slist_base_hook<MySListClass> > myclass_base_list_type;
        typedef intrusive_slist<MySListClass, slist_member_hook<MySListClass, &MySListClass::m_listHook> > myclass_member_list_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySListClass, 20> myclassArray;

        myclass_base_list_type      myclass_base_list;
        myclass_base_list_type      myclass_base_list2;
        myclass_member_list_type    myclass_member_list;
        myclass_member_list_type    myclass_member_list2;
        // merge
        // 2 sorted lists for merge
        array<MySListClass, 4> toMergeArray = {
            {1, 10, 50, 200}
        };
        array<MySListClass, 4> toMergeArray2 = {
            {2, 8, 60, 180}
        };

        myclass_base_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_base_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        myclass_base_list.merge(myclass_base_list2);
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != myclass_base_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data < (*next(iter)).m_data);
        }

        myclass_base_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_base_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        // reverse the sort order
        myclass_base_list.reverse();
        myclass_base_list2.reverse();

        myclass_base_list.merge(myclass_base_list2, AZStd::greater<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_base_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_base_list2);
        for (myclass_base_list_type::iterator iter = myclass_base_list.begin(); iter != myclass_base_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data > (*next(iter)).m_data);
        }

        myclass_member_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_member_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        myclass_member_list.merge(myclass_member_list2);
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != myclass_member_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data < (*next(iter)).m_data);
        }

        myclass_member_list.assign(toMergeArray.begin(), toMergeArray.end());
        myclass_member_list2.assign(toMergeArray2.begin(), toMergeArray2.end());

        // reverse the sort order
        myclass_member_list.reverse();
        myclass_member_list2.reverse();

        myclass_member_list.merge(myclass_member_list2, AZStd::greater<MySListClass>());
        AZ_TEST_VALIDATE_LIST(myclass_member_list, 8);
        AZ_TEST_VALIDATE_EMPTY_LIST(myclass_member_list2);
        for (myclass_member_list_type::iterator iter = myclass_member_list.begin(); iter != myclass_member_list.last(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data > (*next(iter)).m_data);
        }

        myclass_base_list.clear();
        myclass_base_list2.clear();
        myclass_member_list.clear();
        myclass_member_list2.clear();
    }

    TEST_F(IntrusiveListContainers, ReverseIterator)
    {
        using myclass_base_list_type = intrusive_list<MyListClass, list_base_hook<MyListClass> >;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        constexpr size_t arraySize = 20;
        array<MyListClass, arraySize> myclassArray;
        myclass_base_list_type      myclass_base_list;

        FillArray(myclassArray);

        myclass_base_list.assign(myclassArray.begin(), myclassArray.end());

        // iterate in reverse order
        auto myClassReverseBeginIt = myclass_base_list.crbegin();
        auto myClassReverseEndIt = myclass_base_list.crend();
        EXPECT_NE(myClassReverseEndIt, myClassReverseBeginIt);
        // Reverse iterator must be decremented that in order the base() iterator call
        // to return an iterator to a valid element
        ++myClassReverseBeginIt;
        size_t reverseArrayIndex = arraySize - 1;
        for (; myClassReverseBeginIt != myClassReverseEndIt; ++myClassReverseBeginIt, --reverseArrayIndex)
        {
            EXPECT_EQ(reverseArrayIndex * 100 + 101, myClassReverseBeginIt.base()->m_data);
        }
        // The crend() element base() call should refer to the first element of the array which
        // should have a value of 101 + arrayIndex * 100, where arrayIndex is 0.
        EXPECT_EQ(101, myClassReverseBeginIt.base()->m_data);

        {
            // Test empty intrusive_list container case
            myclass_base_list.clear();
            auto reverseIterBegin = myclass_base_list.rbegin();
            auto reverseIterEnd = myclass_base_list.rend();
            EXPECT_EQ(reverseIterBegin, reverseIterEnd);
        }
    }
}

#undef AZ_TEST_VALIDATE_EMPTY_LIST
#undef AZ_TEST_VALIDATE_LIST

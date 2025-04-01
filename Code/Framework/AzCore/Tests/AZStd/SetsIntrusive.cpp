/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/std/functional.h>

#include <AzCore/std/containers/array.h>

#define AZ_TEST_VALIDATE_EMPTY_SET(_set)       \
    EXPECT_EQ(0, _set.size());                 \
    EXPECT_TRUE(_set.begin() == _set.end());   \
    EXPECT_TRUE(_set.rbegin() == _set.rend()); \
    EXPECT_TRUE(_set.empty());

#define AZ_TEST_VALIDATE_SET(_set, _NumElements)                                               \
    EXPECT_EQ(_NumElements, _set.size());                                                      \
    EXPECT_TRUE((_NumElements > 0) ? !_set.empty() : _set.empty());                            \
    EXPECT_TRUE((_NumElements > 0) ? _set.begin() != _set.end() : _set.begin() == _set.end()); \
    EXPECT_FALSE(_set.empty());

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    // My intrusive set class.
    // We have 2 hooks in this class. One of each supported type. Base hook which we inherit from the intrusive_set_node node
    // and public member hook (m_setHook).
    struct MySetClass
        : public intrusive_multiset_node<MySetClass>
    {
    public:
        MySetClass(int data = 101)
            : m_data(data) {}

        intrusive_multiset_node<MySetClass>    m_setHook;     // Public member hook.

        int m_data; // This is left public only for testing purpose.
    };
    // Compare operators, used for sort, merge, etc.
    AZ_FORCE_INLINE bool operator==(const MySetClass& a, const MySetClass& b) { return a.m_data == b.m_data; }
    AZ_FORCE_INLINE bool operator!=(const MySetClass& a, const MySetClass& b) { return a.m_data != b.m_data; }
    AZ_FORCE_INLINE bool operator<(const MySetClass& a, const MySetClass& b) { return a.m_data < b.m_data; }
    AZ_FORCE_INLINE bool operator>(const MySetClass& a, const MySetClass& b) { return a.m_data > b.m_data; }

    class IntrusiveSetContainers
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

    TEST_F(IntrusiveSetContainers, CtorAssign)
    {
        // Create 2 set types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_multiset<MySetClass, intrusive_multiset_base_hook<MySetClass> > myclass_base_set_type;
        typedef intrusive_multiset<MySetClass, intrusive_multiset_member_hook<MySetClass, &MySetClass::m_setHook> > myclass_member_set_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySetClass, 20> myclassArray;
        myclass_base_set_type      myclass_base_set;
        myclass_member_set_type    myclass_member_set;

        // default ctor
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set);      
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_member_set);

        myclass_base_set_type myclass_base_set21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_base_set21, myclassArray.size());
        for (myclass_base_set_type::const_iterator iter = myclass_base_set21.begin(); iter != myclass_base_set21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        myclass_member_set_type myclass_member_set21(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_member_set21, myclassArray.size());
        for (myclass_member_set_type::const_iterator iter = myclass_member_set21.begin(); iter != myclass_member_set21.end(); ++iter)
        {
            AZ_TEST_ASSERT((*iter).m_data == 101);
        }

        // assign
        myclass_base_set21.clear();
        myclass_base_set.insert(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size());

        myclass_member_set21.clear();
        myclass_member_set.insert(myclassArray.begin(), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size());
    }

    TEST_F(IntrusiveSetContainers, SetResizeInsertEraseClear)
    {
        // Create 2 set types, one which hooks to the base hook and one to the public member hook.
        typedef intrusive_multiset<MySetClass, intrusive_multiset_base_hook<MySetClass> > myclass_base_set_type;
        typedef intrusive_multiset<MySetClass, intrusive_multiset_member_hook<MySetClass, &MySetClass::m_setHook> > myclass_member_set_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySetClass, 20> myclassArray;
        myclass_base_set_type      myclass_base_set;
        myclass_member_set_type    myclass_member_set;

        // insert
        myclass_base_set.clear();
        myclass_member_set.clear();

        myclass_base_set.insert(*myclassArray.begin());
        AZ_TEST_VALIDATE_SET(myclass_base_set, 1);
        AZ_TEST_ASSERT(myclass_base_set.begin()->m_data == myclassArray.begin()->m_data);

        myclass_member_set.insert(*myclassArray.begin());
        AZ_TEST_VALIDATE_SET(myclass_member_set, 1);
        AZ_TEST_ASSERT(myclass_member_set.begin()->m_data == myclassArray.begin()->m_data);

        myclass_base_set.insert(next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size());
        AZ_TEST_ASSERT(myclass_base_set.begin()->m_data == prev(myclassArray.end())->m_data);

        myclass_member_set.insert(next(myclassArray.begin()), myclassArray.end());
        AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size());
        AZ_TEST_ASSERT(myclass_member_set.rbegin()->m_data == prev(myclassArray.end())->m_data);

        // erase
        myclass_base_set.erase(prev(myclass_base_set.end()));
        AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size() - 1);
        myclass_base_set.erase(myclass_base_set.begin());
        AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size() - 2);

        myclass_member_set.erase(prev(myclass_member_set.end()));
        AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size() - 1);
        myclass_member_set.erase(myclass_member_set.begin());
        AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size() - 2);

        myclass_base_set.erase(next(myclass_base_set.begin()), myclass_base_set.end());
        AZ_TEST_VALIDATE_SET(myclass_base_set, 1);
        myclass_member_set.erase(next(myclass_member_set.begin()), myclass_member_set.end());
        AZ_TEST_VALIDATE_SET(myclass_member_set, 1);

        // clear
        myclass_base_set.clear();
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set);
        myclass_member_set.clear();
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_member_set);
    }

    // TODO: LY-87175 Add move and swap operations to intrusive_set
    //TEST_F(IntrusiveSetContainers, Swap)
    //{
    //    typedef intrusive_multiset<MySetClass, intrusive_multiset_base_hook<MySetClass> > myclass_base_set_type;
    //    typedef intrusive_multiset<MySetClass, intrusive_multiset_member_hook<MySetClass, &MySetClass::m_setHook> > myclass_member_set_type;

    //    // A static array of MyClass, init with default ctor. Used as source for some of the tests.
    //    array<MySetClass, 20> myclassArray;
    //    myclass_base_set_type      myclass_base_set;
    //    myclass_base_set_type      myclass_base_set2;
    //    myclass_member_set_type    myclass_member_set;
    //    myclass_member_set_type    myclass_member_set2;

    //    // swap
    //    myclass_base_set.insert(myclassArray.begin(), myclassArray.end());
    //    myclass_member_set.insert(myclassArray.begin(), myclassArray.end());

    //    myclass_base_set2.swap(myclass_base_set);
    //    AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set);
    //    AZ_TEST_VALIDATE_SET(myclass_base_set2, myclassArray.size());

    //    myclass_member_set2.swap(myclass_member_set);
    //    AZ_TEST_VALIDATE_EMPTY_SET(myclass_member_set);
    //    AZ_TEST_VALIDATE_SET(myclass_member_set2, myclassArray.size());

    //    myclass_base_set2.swap(myclass_base_set);
    //    AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set2);
    //    AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size());

    //    myclass_member_set2.swap(myclass_member_set);
    //    AZ_TEST_VALIDATE_EMPTY_SET(myclass_member_set2);
    //    AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size());

    //    myclass_base_set.erase(*myclass_base_set.rbegin());
    //    AZ_TEST_VALIDATE_SET(myclass_base_set, myclassArray.size() - 1);
    //    myclass_member_set.erase(*myclass_base_set.rbegin());
    //    AZ_TEST_VALIDATE_SET(myclass_member_set, myclassArray.size() - 1);

    //    myclass_base_set2.insert(myclassArray.back());
    //    myclass_member_set2.insert(myclassArray.back());

    //    myclass_base_set.swap(myclass_base_set2);
    //    AZ_TEST_VALIDATE_SET(myclass_base_set2, myclassArray.size() - 1);
    //    AZ_TEST_VALIDATE_SET(myclass_base_set, 1);

    //    myclass_member_set.swap(myclass_member_set2);
    //    AZ_TEST_VALIDATE_SET(myclass_member_set2, myclassArray.size() - 1);
    //    AZ_TEST_VALIDATE_SET(myclass_member_set, 1);

    //    myclass_base_set.clear();
    //    myclass_base_set2.clear();
    //    myclass_member_set.clear();
    //    myclass_member_set2.clear();
    //}

    TEST_F(IntrusiveSetContainers, RemoveWhileIterating)
    {
        typedef intrusive_multiset<MySetClass, intrusive_multiset_base_hook<MySetClass> > myclass_base_set_type;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        array<MySetClass, 2> myclassArray;
        myclass_base_set_type myclass_base_set;

        FillArray(myclassArray);

        // remove
        myclass_base_set.insert(myclassArray.begin(), myclassArray.end());
        myclass_base_set_type::const_iterator it = myclass_base_set.begin();
        while (it != myclass_base_set.end())
        {
            myclass_base_set.erase(it++);
        }
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set);

        // repopulate and remove in reverse order
        myclass_base_set.insert(myclassArray.begin(), myclassArray.end());
        myclass_base_set_type::const_reverse_iterator rit = myclass_base_set.crbegin();
        while (rit != myclass_base_set.crend())
        {
            const MySetClass* item = rit.operator->();
            ++rit;
            myclass_base_set.erase(item);
        }
        AZ_TEST_VALIDATE_EMPTY_SET(myclass_base_set);
    }

    TEST_F(IntrusiveSetContainers, ReverseIterator)
    {
        using myclass_base_set_type = intrusive_multiset<MySetClass, intrusive_multiset_base_hook<MySetClass> >;

        // A static array of MyClass, init with default ctor. Used as source for some of the tests.
        constexpr size_t arraySize = 20;
        array<MySetClass, arraySize> myclassArray;
        myclass_base_set_type      myclass_base_set;

        FillArray(myclassArray);

        myclass_base_set.insert(myclassArray.begin(), myclassArray.end());

        // iterate in reverse order
        auto myClassReverseBeginIt = myclass_base_set.crbegin();
        auto myClassReverseEndIt = myclass_base_set.crend();
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
            // Test empty intrusive container case
            myclass_base_set.clear();
            auto reverseIterBegin = myclass_base_set.rbegin();
            auto reverseIterEnd = myclass_base_set.rend();
            EXPECT_EQ(reverseIterBegin, reverseIterEnd);
        }
    }
}

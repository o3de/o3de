/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Atom/Feature/Utils/IndexableList.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <gtest/gtest.h>
#include <AzCore/Math/Random.h>
#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class IndexableListTests
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(IndexableListTests, TestBasics)
    {
        IndexableList<float> container;
        EXPECT_EQ(0, container.size());
        EXPECT_EQ(0, container.capacity());
        EXPECT_EQ(-1, container.begin());
    }

    TEST_F(IndexableListTests, TestReserveFromZero)
    {
        IndexableList<float> container;
        container.reserve(1);
        EXPECT_LE(1, container.capacity());
        EXPECT_EQ(0, container.size());
        EXPECT_EQ(-1, container.begin());
    }

    TEST_F(IndexableListTests, TestPushFront)
    {
        const float valueToInsert = 123.25f;

        IndexableList<float> container;
        int position = container.push_front(valueToInsert);
        EXPECT_EQ(1, container.size());
        EXPECT_EQ(valueToInsert, container[position]);
    }

    TEST_F(IndexableListTests, TestErase)
    {
        const float valueToInsert = 123.25f;

        IndexableList<float> container;
        int position = container.push_front(valueToInsert);
        container.erase(position);
        EXPECT_EQ(0, container.size());
    }

    TEST_F(IndexableListTests, TestBegin)
    {
        const int testValue = 123;
        IndexableList<int> container;
        container.push_front(testValue);
        int listHead = container.begin();
        EXPECT_EQ(testValue, container[listHead]);
    }

    TEST_F(IndexableListTests, TestNextOnce)
    {
        const int testValue0 = 123;
        const int testValue1 = 456;

        IndexableList<int> container;
        container.push_front(testValue0);
        container.push_front(testValue1);

        int iterator = container.begin();
        iterator = container.next(iterator);
        EXPECT_EQ(testValue0, container[iterator]);
    }

    TEST_F(IndexableListTests, TestNextMultiple)
    {
        IndexableList<int> container;
        container.push_front(0);
        container.push_front(1);
        int element2 = container.push_front(2);
        container.push_front(3);
        container.erase(element2);

        int numItemsIteratedThrough = 0;
        int iterator = container.begin();
        while (iterator != -1)
        {
            iterator = container.next(iterator);
            numItemsIteratedThrough++;
        }
        EXPECT_EQ(numItemsIteratedThrough, container.size());
    }

    TEST_F(IndexableListTests, TestMultipleReserve)
    {
        const int testValue0 = -9;
        const int testValue1 = 65;
        const int testValue2 = 32;

        IndexableList<int> container;
        container.reserve(2);
        int element0 = container.push_front(testValue0);
        container.reserve(4);
        int element1 = container.push_front(testValue1);
        container.reserve(6);
        int element2 = container.push_front(testValue2);
        EXPECT_LE(6, container.capacity());
        EXPECT_EQ(3, container.size());

        EXPECT_EQ(testValue0, container[element0]);
        EXPECT_EQ(testValue1, container[element1]);
        EXPECT_EQ(testValue2, container[element2]);
    }

    TEST_F(IndexableListTests, TestInsertToMaxThenReserve)
    {
        const int testValue0 = -9;
        const int testValue1 = 65;
        const int testValue2 = 32;
        const int testValue3 = 0;
        const int testValue4 = -1;
        const int testValue5 = 2;

        IndexableList<int> container;
        container.reserve(3);
        int element0 = container.push_front(testValue0);
        int element1 = container.push_front(testValue1);
        int element2 = container.push_front(testValue2);
        container.reserve(6);
        int element3 = container.push_front(testValue3);
        int element4 = container.push_front(testValue4);
        int element5 = container.push_front(testValue5);

        EXPECT_EQ(testValue0, container[element0]);
        EXPECT_EQ(testValue1, container[element1]);
        EXPECT_EQ(testValue2, container[element2]);
        EXPECT_EQ(testValue3, container[element3]);
        EXPECT_EQ(testValue4, container[element4]);
        EXPECT_EQ(testValue5, container[element5]);
    }

    TEST_F(IndexableListTests, TestHolesInList)
    {
        const int testValue0 = -9;
        const int testValue1 = 65;
        const int testValue2 = 32;
        const int testValue3 = 0;
        const int testValue4 = -1;
        const int testValue5 = 2;

        IndexableList<int> container;
        int element0 = container.push_front(testValue0);
        int element1 = container.push_front(testValue1);
        int element2 = container.push_front(testValue2);
        container.erase(element1);
        int element3 = container.push_front(testValue3);
        int element4 = container.push_front(testValue4);
        int element5 = container.push_front(testValue5);
        container.erase(element4);

        EXPECT_EQ(4, container.size());

        EXPECT_EQ(testValue0, container[element0]);
        EXPECT_EQ(testValue2, container[element2]);
        EXPECT_EQ(testValue3, container[element3]);
        EXPECT_EQ(testValue5, container[element5]);
    }

    TEST_F(IndexableListTests, TestArraySize)
    {
        const int testValue0 = 5;

        IndexableList<int> container;
        int element0 = container.push_front(testValue0);
        container.erase(element0);

        EXPECT_LE(container.size(), container.array_size());
        EXPECT_LE(1, container.array_size());
    }

    TEST_F(IndexableListTests, ClearTest)
    {
        const int testValue = 5;

        IndexableList<int> container;
        container.push_front(testValue);
        EXPECT_EQ(1, container.size());

        container.clear();

        EXPECT_EQ(0, container.size());
        EXPECT_EQ(-1, container.begin());
    }
}

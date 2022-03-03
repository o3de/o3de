/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <gtest/gtest.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;
    
    class IndexedDataVectorTests
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            UnitTest::AllocatorsTestFixture::TearDown();
        }

        template<typename T>
        IndexedDataVector<T> SetupIndexedDataVector(size_t size, T initialValue = T(0), T incrementAmount = T(1), AZStd::vector<uint16_t>* indices = nullptr)
        {
            IndexedDataVector<T> data;
            T value = initialValue;
            for (size_t i = 0; i < size; ++i)
            {
                uint16_t index = data.GetFreeSlotIndex();
                EXPECT_NE(index, IndexedDataVector<int>::NoFreeSlot);
                if (indices)
                {
                    indices->push_back(index);
                }
                if (index != IndexedDataVector<int>::NoFreeSlot)
                {
                    data.GetData(index) = value;
                    value += incrementAmount;
                }
            }
            return data;
        }

        template<typename T>
        void ShuffleIndexedDataVector(IndexedDataVector<T>& dataVector, AZStd::vector<uint16_t>& indices)
        {
            AZStd::vector<T> values;

            // remove every other element and store it
            for (size_t i = 0; i < indices.size(); ++i)
            {
                values.push_back(dataVector.GetData(indices.at(i)));
                dataVector.RemoveIndex(indices.at(i));
                indices.erase(&indices.at(i));
            }

            for (T value : values)
            {
                uint16_t index = dataVector.GetFreeSlotIndex();
                indices.push_back(index);
                dataVector.GetData(index) = value;
            }
        }

    };
    
    TEST_F(IndexedDataVectorTests, Construction)
    {
        IndexedDataVector<int> testVector;
        uint16_t index = testVector.GetFreeSlotIndex();
        EXPECT_NE(index, IndexedDataVector<int>::NoFreeSlot);
    }
    
    TEST_F(IndexedDataVectorTests, TestInsertGetBasic)
    {
        constexpr size_t count = 16;
        constexpr int initialValue = 0;
        constexpr int increment = 1;

        AZStd::vector<uint16_t> indices;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count, initialValue, increment, &indices);
        
        int value = initialValue;
        for (size_t i = 0; i < count; ++i)
        {
            EXPECT_EQ(testVector.GetData(indices.at(i)), value);
            value += increment;
        }
    }
    
    TEST_F(IndexedDataVectorTests, TestInsertGetComplex)
    {
        constexpr size_t count = 16;
        constexpr int initialValue = 0;
        constexpr int increment = 1;

        AZStd::vector<uint16_t> indices;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count, initialValue, increment, &indices);

        // Create a set of the data that should be in the IndexedDataVector
        AZStd::set<int> values;
        for (int i = 0; i < count; ++i)
        {
            values.emplace(initialValue + i * increment);
        }

        // Add and remove items to shuffle the underlying data
        ShuffleIndexedDataVector(testVector, indices);

        // Check to make sure all the data is still there
        AZStd::vector<int>& underlyingVector = testVector.GetDataVector();
        for (size_t i = 0; i < underlyingVector.size(); ++i)
        {
            EXPECT_TRUE(values.contains(underlyingVector.at(i)));
        }
    }

    TEST_F(IndexedDataVectorTests, TestSize)
    {
        constexpr size_t count = 32;

        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count);
        EXPECT_EQ(testVector.GetDataCount(), count);
    }

    TEST_F(IndexedDataVectorTests, TestClear)
    {
        constexpr size_t count = 32;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count);
        testVector.Clear();
        EXPECT_EQ(testVector.GetDataCount(), 0);
    }

    TEST_F(IndexedDataVectorTests, TestRemove)
    {
        constexpr size_t count = 8;
        constexpr int initialValue = 0;
        constexpr int increment = 8;

        AZStd::vector<uint16_t> indices;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count, initialValue, increment, &indices);

        // Remove every other element by index
        for (uint16_t i = 0; i < count; i += 2)
        {
            testVector.RemoveIndex(i);
        }
        
        EXPECT_EQ(testVector.GetDataCount(), count / 2);

        // Make sure the rest of the data is still there
        AZStd::vector<uint16_t> remainingIndices;
        for (size_t i = 1; i < count; i += 2)
        {
            int value = testVector.GetData(indices.at(i));
            EXPECT_EQ(value, initialValue + increment * i);
            remainingIndices.push_back(indices.at(i));
        }

        // remove the rest of the valus by value
        for (uint16_t index : remainingIndices)
        {
            int* valuePtr = &testVector.GetData(index);
            testVector.RemoveData(valuePtr);
        }

        EXPECT_EQ(testVector.GetDataCount(), 0);
    }

    TEST_F(IndexedDataVectorTests, TestIndexForData)
    {
        constexpr size_t count = 8;
        constexpr int initialValue = 0;
        constexpr int increment = 8;

        AZStd::vector<uint16_t> indices;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count, initialValue, increment, &indices);
        
        // Add and remove items to shuffle the underlying data
        ShuffleIndexedDataVector(testVector, indices);
        
        AZStd::vector<int>& underlyingVector = testVector.GetDataVector();
        for (size_t i = 0; i < underlyingVector.size(); ++i)
        {
            int value = underlyingVector.at(i);
            uint16_t index = testVector.GetIndexForData(&underlyingVector.at(i));

            // The data from GetData(index) should match for the index retrieved using GetIndexForData() for the same data.
            EXPECT_EQ(testVector.GetData(index), value);
        }
    }
    
    TEST_F(IndexedDataVectorTests, TestRawIndex)
    {
        constexpr size_t count = 8;
        constexpr int initialValue = 0;
        constexpr int increment = 8;

        AZStd::vector<uint16_t> indices;
        IndexedDataVector<int> testVector = SetupIndexedDataVector<int>(count, initialValue, increment, &indices);

        // Add and remove items to shuffle the underlying data
        ShuffleIndexedDataVector(testVector, indices);
        
        AZStd::vector<int>& underlyingVector = testVector.GetDataVector();
        for (size_t i = 0; i < indices.size(); ++i)
        {
            // Check that the data retrieved from GetData for a given index matches the data in the underlying vector for the raw index.
            EXPECT_EQ(testVector.GetData(indices.at(i)), underlyingVector.at(testVector.GetRawIndex(indices.at(i))));
        }

    }
}

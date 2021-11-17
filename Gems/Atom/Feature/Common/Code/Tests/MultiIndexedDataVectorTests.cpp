/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <gtest/gtest.h>


namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class MultiIndexedDataVectorTests
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
    };

    TEST_F(MultiIndexedDataVectorTests, TestInsert)
    {
        enum Types
        {
            IntType = 0,
            DoubleType = 1,
        };

        MultiIndexedDataVector<int, double> myVec;
        constexpr int NumToInsert = 5;

        AZStd::vector<uint16_t> indices;

        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            indices.push_back(index);
            myVec.GetData<IntType>(index) = i;
            myVec.GetData<DoubleType>(index) = (double)i;
        }

        for (size_t i = 0; i < NumToInsert; ++i)
        {
            auto index = indices[i];
            EXPECT_EQ(i, myVec.GetData<IntType>(index));
            EXPECT_EQ((double)i, myVec.GetData<DoubleType>(index));
        }
    }

    TEST_F(MultiIndexedDataVectorTests, TestSize)
    {
        enum Types
        {
            IntType = 0,
        };

        MultiIndexedDataVector<int> myVec;
        constexpr int NumToInsert = 5;
        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            myVec.GetData<IntType>(index) = i;
        }
        EXPECT_EQ(NumToInsert, myVec.GetDataCount());
        EXPECT_EQ(NumToInsert, myVec.GetDataVector<IntType>().size());

        myVec.Clear();

        EXPECT_EQ(0, myVec.GetDataCount());
        EXPECT_EQ(0, myVec.GetDataVector<IntType>().size());
    }

    TEST_F(MultiIndexedDataVectorTests, TestErase)
    {
        enum Types
        {
            IntType = 0,
        };

        MultiIndexedDataVector<int> myVec;
        constexpr int NumToInsert = 200;
        AZStd::unordered_map<int, uint16_t> valueToIndex;

        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            valueToIndex[i] = index;
            myVec.GetData<IntType>(index) = i;
        }

        // erase every even number
        for (int i = 0; i < NumToInsert; i += 2)
        {
            uint16_t index = valueToIndex[i];
            auto previousRawIndex = myVec.GetRawIndex(index);
            auto movedIndex = myVec.RemoveIndex(index);
            if (movedIndex != MultiIndexedDataVector<int>::NoFreeSlot)
            {
                auto newRawIndex = myVec.GetRawIndex(movedIndex);

                // RemoveIndex() returns the index of the item that moves into its spot if any, so check
                // to make sure the Raw index of the old matches the raw index of the new
                EXPECT_EQ(previousRawIndex, newRawIndex);
            }
            valueToIndex.erase(i);
        }

        for (const auto& iter : valueToIndex)
        {
            int val = iter.first;
            uint16_t index = iter.second;
            EXPECT_EQ(val, myVec.GetData<IntType>(index));
        }
    }

    TEST_F(MultiIndexedDataVectorTests, TestManyTypes)
    {
        enum Types
        {
            IntType = 0,
            StringType = 1,
            DoubleType = 2,
            FloatType = 3,
            CharType = 4,
        };

        MultiIndexedDataVector<int, AZStd::string, double, float, const char*> myVec;
        auto index = myVec.GetFreeSlotIndex();

        constexpr int TestIntVal = INT_MIN;
        constexpr double TestDoubleVal = -DBL_MIN;
        const AZStd::string TestStringVal = "This is an AZStd::string.";
        constexpr float TestFloatVal = FLT_MAX;
        const char* TestConstPointerVal = "This is a C array.";

        myVec.GetData<IntType>(index) = TestIntVal;
        myVec.GetData<StringType>(index) = TestStringVal;
        myVec.GetData<DoubleType>(index) = TestDoubleVal;
        myVec.GetData<FloatType>(index) = TestFloatVal;
        myVec.GetData<CharType>(index) = TestConstPointerVal;

        EXPECT_EQ(TestIntVal, static_cast<int>(myVec.GetData<IntType>(index)));
        EXPECT_EQ(TestStringVal, static_cast<AZStd::string>(myVec.GetData<StringType>(index)));
        EXPECT_EQ(TestDoubleVal, static_cast<double>(myVec.GetData<DoubleType>(index)));
        EXPECT_EQ(TestFloatVal, static_cast<float>(myVec.GetData<FloatType>(index)));
        EXPECT_STREQ(TestConstPointerVal, static_cast<const char*>(myVec.GetData<CharType>(index)));
    }

    MultiIndexedDataVector<int32_t, float> CreateTestVector(AZStd::vector<uint16_t>& indices)
    {
        enum Types
        {
            IntType = 0,
            FloatType = 1,
        };

        MultiIndexedDataVector<int32_t, float> myVec;
        constexpr int32_t Count = 10;
        int32_t startInt = 10;
        float startFloat = 2.0f;
        
        // Create some initial values
        for (uint32_t i = 0; i < Count; ++i)
        {
            uint16_t index = myVec.GetFreeSlotIndex();
            indices.push_back(index);
            myVec.GetData<IntType>(index) = startInt;
            myVec.GetData<FloatType>(index) = startFloat;
            startInt += 1;
            startFloat += 1.0f;
        }

        return myVec;
    }
    
    void CheckIndexedData(MultiIndexedDataVector<int32_t, float>& data, AZStd::vector<uint16_t>& indices)
    {
        enum Types
        {
            IntType = 0,
            FloatType = 1,
        };

        // For each index, get its data and make sure GetIndexForData returns the same
        // index used to retrieve the data
        for (uint32_t i = 0; i < data.GetDataCount(); ++i)
        {
            int32_t& intData = data.GetData<IntType>(indices.at(i));
            uint16_t indexForData = data.GetIndexForData<IntType>(&intData);
            EXPECT_EQ(indices.at(i), indexForData);
                
            float& floatData = data.GetData<FloatType>(indices.at(i));
            indexForData = data.GetIndexForData<FloatType>(&floatData);
            EXPECT_EQ(indices.at(i), indexForData);
        }
    }

    TEST_F(MultiIndexedDataVectorTests, GetIndexForDataSimple)
    {
        AZStd::vector<uint16_t> indices;
        MultiIndexedDataVector<int32_t, float> myVec = CreateTestVector(indices);
        CheckIndexedData(myVec, indices);
    }

    TEST_F(MultiIndexedDataVectorTests, GetIndexForDataComplex)
    {
        enum Types
        {
            IntType = 0,
            FloatType = 1,
        };

        AZStd::vector<uint16_t> indices;
        MultiIndexedDataVector<int32_t, float> myVec = CreateTestVector(indices);

        // remove every other value to shuffle the data around
        for (uint32_t i = 0; i < myVec.GetDataCount(); i += 2)
        {
            myVec.RemoveIndex(indices.at(i));
        }
        
        int32_t startInt = 100;
        float startFloat = 20.0f;

        // Add some data back in
        const size_t count = myVec.GetDataCount();
        for (uint32_t i = 0; i < count; i += 2)
        {
            uint16_t index = myVec.GetFreeSlotIndex();
            indices.at(i) = index;
            myVec.GetData<IntType>(index) = startInt;
            myVec.GetData<FloatType>(index) = startFloat;
            startInt += 1;
            startFloat += 1.0f;
        }

        CheckIndexedData(myVec, indices);
    }

    TEST_F(MultiIndexedDataVectorTests, ForEach)
    {
        enum Types
        {
            IntType = 0,
            FloatType = 1,
        };

        MultiIndexedDataVector<int32_t, float> myVec;
        constexpr int32_t Count = 10;
        int32_t startInt = 10;
        float startFloat = 2.0f;

        AZStd::vector<uint16_t> indices;
        AZStd::set<int32_t> intValues;
        AZStd::set<float> floatValues;

        // Create some initial values
        for (uint32_t i = 0; i < Count; ++i)
        {
            uint16_t index = myVec.GetFreeSlotIndex();
            indices.push_back(index);
            myVec.GetData<IntType>(index) = startInt;
            myVec.GetData<FloatType>(index) = startFloat;
            intValues.insert(startInt);
            floatValues.insert(startFloat);
            startInt += 1;
            startFloat += 1.0f;
        }

        uint32_t visitCount = 0;
        myVec.ForEach<IntType>([&](int32_t value) -> bool
        {
            intValues.erase(value);
            ++visitCount;
            return true; // keep iterating
        });

        // All ints should have been visited and found in the set
        EXPECT_EQ(visitCount, Count);
        EXPECT_EQ(intValues.size(), 0);

        visitCount = 0;
        myVec.ForEach<FloatType>([&](float value) -> bool
        {
            floatValues.erase(value);
            ++visitCount;
            return true; // keep iterating
        });
        
        // All floats should have been visited and found in the set
        EXPECT_EQ(visitCount, Count);
        EXPECT_EQ(floatValues.size(), 0);
        
        visitCount = 0;
        myVec.ForEach<IntType>([&]([[maybe_unused]] int32_t value) -> bool
        {
            ++visitCount;
            return false; // stop iterating
        });

        // Since false is immediately returned, only one element should have been visited.
        EXPECT_EQ(visitCount, 1);

    }
}

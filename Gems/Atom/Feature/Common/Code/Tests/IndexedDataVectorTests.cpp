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

    class IndexedDataVectorTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            CreateAllocator();
        }

        void TearDown() override
        {
            DestroyAllocator();
        }

    private:

        void CreateAllocator()
        {
            static constexpr size_t NumMBToAllocate = 1;
            SystemAllocator::Descriptor desc;
            desc.m_heap.m_numFixedMemoryBlocks = 1;
            desc.m_heap.m_fixedMemoryBlocksByteSize[0] = NumMBToAllocate * 1024 * 1024;
            m_memBlock = AZ_OS_MALLOC(
                desc.m_heap.m_fixedMemoryBlocksByteSize[0],
                desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_fixedMemoryBlocks[0] = m_memBlock;

            AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        void DestroyAllocator()
        {
            AllocatorInstance<AZ::SystemAllocator>::Destroy();
            AZ_OS_FREE(m_memBlock);
            m_memBlock = nullptr;
        }

        void* m_memBlock = nullptr;
    };

    TEST_F(IndexedDataVectorTests, TestInsert)
    {
        MultiIndexedDataVector<int, double> myVec;
        constexpr int NumToInsert = 5;

        AZStd::vector<uint16_t> indices;

        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            indices.push_back(index);
            myVec.GetData<0>(index) = i;
            myVec.GetData<1>(index) = (double)i;
        }

        for (size_t i = 0; i < NumToInsert; ++i)
        {
            auto index = indices[i];
            EXPECT_EQ(i, myVec.GetData<0>(index));
            EXPECT_EQ((double)i, myVec.GetData<1>(index));
        }
    }

    TEST_F(IndexedDataVectorTests, TestSize)
    {
        MultiIndexedDataVector<int> myVec;
        constexpr int NumToInsert = 5;
        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            myVec.GetData<0>(index) = i;
        }
        EXPECT_EQ(NumToInsert, myVec.GetDataCount());
        EXPECT_EQ(NumToInsert, myVec.GetDataVector<0>().size());

        myVec.Clear();

        EXPECT_EQ(0, myVec.GetDataCount());
        EXPECT_EQ(0, myVec.GetDataVector<0>().size());
    }

    TEST_F(IndexedDataVectorTests, TestErase)
    {
        MultiIndexedDataVector<int> myVec;
        constexpr int NumToInsert = 200;
        AZStd::unordered_map<int, uint16_t> valueToIndex;

        for (int i = 0; i < NumToInsert; ++i)
        {
            auto index = myVec.GetFreeSlotIndex();
            valueToIndex[i] = index;
            myVec.GetData<0>(index) = i;
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
            EXPECT_EQ(val, myVec.GetData<0>(index));
        }
    }

    TEST_F(IndexedDataVectorTests, TestManyTypes)
    {
        MultiIndexedDataVector<int, AZStd::string, double, float, const char*> myVec;
        auto index = myVec.GetFreeSlotIndex();

        constexpr int TestIntVal = INT_MIN;
        constexpr double TestDoubleVal = -DBL_MIN;
        const AZStd::string TestStringVal = "This is an AZStd::string.";
        constexpr float TestFloatVal = FLT_MAX;
        const char* TestConstPointerVal = "This is a C array.";

        myVec.GetData<0>(index) = TestIntVal;
        myVec.GetData<1>(index) = TestStringVal;
        myVec.GetData<2>(index) = TestDoubleVal;
        myVec.GetData<3>(index) = TestFloatVal;
        myVec.GetData<4>(index) = TestConstPointerVal;

        EXPECT_EQ(TestIntVal, static_cast<int>(myVec.GetData<0>(index)));
        EXPECT_EQ(TestStringVal, static_cast<AZStd::string>(myVec.GetData<1>(index)));
        EXPECT_EQ(TestDoubleVal, static_cast<double>(myVec.GetData<2>(index)));
        EXPECT_EQ(TestFloatVal, static_cast<float>(myVec.GetData<3>(index)));
        EXPECT_STREQ(TestConstPointerVal, static_cast<const char*>(myVec.GetData<4>(index)));
    }
}

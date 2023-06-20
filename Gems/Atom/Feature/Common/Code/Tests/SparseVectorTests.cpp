/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/Feature/Utils/SparseVector.h>
#include <Atom/Feature/Utils/MultiSparseVector.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    class SparseVectorTests
        : public UnitTest::LeakDetectionFixture
    {
    };

    struct TestData
    {
        static constexpr int DefaultValueA = 100;
        static constexpr float DefaultValueB = 123.45f;
        static constexpr bool DefaultValueC = true;

        int a = DefaultValueA;
        float b = DefaultValueB;
        bool c = DefaultValueC;
    };

    struct TestDataWithDestructor
    {
        bool m_destroyed = false;
        size_t m_value = 100;

        ~TestDataWithDestructor()
        {
            EXPECT_FALSE(m_destroyed);
            m_destroyed = true;
        }
    };

    TEST_F(SparseVectorTests, SparseVectorCreate)
    {
        // Simple test to make sure we can create a SparseVector and it initializes with no values.
        SparseVector<TestData> container;
        EXPECT_EQ(0, container.GetSize());
        container.Clear();
        EXPECT_EQ(0, container.GetSize());
    }
    
    TEST_F(SparseVectorTests, SparseVectorReserveRelease)
    {
        SparseVector<TestData> container;
        constexpr size_t Count = 10;
        size_t indices[Count];

        // Create some elements
        for (size_t i = 0; i < Count; ++i)
        {
            indices[i] = container.Reserve();
        }
        
        EXPECT_EQ(container.GetSize(), Count);
        
        // Ensure that the elements were created with valid indices
        for (size_t i = 0; i < Count; ++i)
        {
            EXPECT_EQ(indices[i], i);
        }

        // Check default initialization of struct and initialize primitive types
        for (size_t i = 0; i < Count; ++i)
        {
            TestData& data = container.GetElement(indices[i]);
            
            // Ensure that the data was initialized properly.
            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);

            // Assign new unique values
            data.a = TestData::DefaultValueA * static_cast<int>(i);
            data.b = TestData::DefaultValueB * float(i);
            data.c = i % 2 == 0;
        }

        // Release every other element.
        for (size_t i = 0; i < Count; i += 2)
        {
            container.Release(indices[i]);
        }

        // Size should be unaffected by release since it just leaves empty slots.
        EXPECT_EQ(container.GetSize(), Count);

        // Check the remaining slots to make sure the data is still correct
        for (size_t i = 1; i < Count; i += 2)
        {
            TestData& data = container.GetElement(indices[i]);
            
            EXPECT_EQ(data.a, TestData::DefaultValueA * i);
            EXPECT_EQ(data.b, TestData::DefaultValueB * float(i));
            EXPECT_EQ(data.c, i % 2 == 0);
        }

        // Re-reserve the previously deleted elements
        for (size_t i = 0; i < Count; i += 2)
        {
            indices[i] = container.Reserve();
        }

        // Make sure the new elements data is initialized to default.
        for (size_t i = 0; i < Count; i += 2)
        {
            TestData& data = container.GetElement(indices[i]);
            
            // Ensure that the data was initialized properly.
            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);
        }

    }

    TEST_F(SparseVectorTests, SparseVectorGetRawData)
    {
        SparseVector<TestData> container;
        constexpr size_t Count = 10;
        [[maybe_unused]] size_t indices[Count];

        // Create some elements
        for (size_t i = 0; i < Count; ++i)
        {
            indices[i] = container.Reserve();
        }

        // Get the raw data pointer
        const TestData* testData = container.GetRawData();

        // Make sure the data in the raw array matches what's expected.
        for (size_t i = 0; i < container.GetSize(); ++i)
        {
            const TestData& data = testData[i];
            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);
        }

        container.Clear();
        EXPECT_EQ(0, container.GetSize());
    }

    TEST_F(SparseVectorTests, MultiSparseVectorCreate)
    {
        // Simple test to make sure we can create a MultiSparseVector and it initializes with no values.
        MultiSparseVector<TestData, int, float> container;
        EXPECT_EQ(0, container.GetSize());
        container.Clear();
        EXPECT_EQ(0, container.GetSize());
    }
    
    TEST_F(SparseVectorTests, MultiSparseVectorReserve)
    {
        MultiSparseVector<TestData, int, float> container;
        constexpr size_t Count = 10;
        size_t indices[Count];

        // Create some elements
        for (size_t i = 0; i < Count; ++i)
        {
            indices[i] = container.Reserve();
        }
        
        EXPECT_EQ(container.GetSize(), Count);
        
        // Ensure that the elements were created with valid indices
        for (size_t i = 0; i < Count; ++i)
        {
            EXPECT_EQ(indices[i], i);
        }
        
        // Ensure that the data was initialized properly.
        for (size_t i = 0; i < Count; ++i)
        {
            TestData& data = container.GetElement<0>(indices[i]);

            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);
            
            data.a = TestData::DefaultValueA * static_cast<int>(i);
            data.b = TestData::DefaultValueB * float(i);
            data.c = i % 2 == 0;

            // Assign some values to the uninitialized primitive types
            container.GetElement<1>(indices[i]) = static_cast<int>(i * 10);
            container.GetElement<2>(indices[i]) = i * 20.0f;
        }

        // Release every other element
        for (size_t i = 0; i < Count; i += 2)
        {
            container.Release(indices[i]);
        }

        // Size should be unaffected by release since it just leaves empty slots.
        EXPECT_EQ(container.GetSize(), Count);

        // Check the remaining slots to make sure the data is still correct
        for (size_t i = 1; i < Count; i += 2)
        {
            TestData& data = container.GetElement<0>(indices[i]);
            
            EXPECT_EQ(data.a, TestData::DefaultValueA * i);
            EXPECT_EQ(data.b, TestData::DefaultValueB * float(i));
            EXPECT_EQ(data.c, i % 2 == 0);
            
            EXPECT_EQ(container.GetElement<1>(indices[i]), i * 10);
            EXPECT_EQ(container.GetElement<2>(indices[i]), i * 20.0f);
        }

        // Re-reserve the previously deleted elements
        for (size_t i = 0; i < Count; i += 2)
        {
            indices[i] = container.Reserve();
        }

        // Make sure the new elements data is initialized to default.
        for (size_t i = 0; i < Count; i += 2)
        {
            TestData& data = container.GetElement<0>(indices[i]);
            
            // Ensure that the data was initialized properly.
            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);
            
            EXPECT_EQ(container.GetElement<1>(indices[i]), 0);
            EXPECT_EQ(container.GetElement<2>(indices[i]), 0.0f);
        }

    }

    TEST_F(SparseVectorTests, MultiSparseVectorGetRawData)
    {
        MultiSparseVector<TestData, int, float> container;
        constexpr size_t Count = 10;
        [[maybe_unused]] size_t indices[Count];

        // Create some elements and give them values to check later.
        for (size_t i = 0; i < Count; ++i)
        {
            indices[i] = container.Reserve();

            container.GetElement<1>(i) = static_cast<int>(i * 10);
            container.GetElement<2>(i) = i * 20.0f;
        }

        // Get raw data arrays
        const TestData* testData = container.GetRawData<0>();
        const int* testints = container.GetRawData<1>();
        const float* testfloats = container.GetRawData<2>();

        // Check all the data to make sure it's accurate.
        for (size_t i = 0; i < container.GetSize(); ++i)
        {
            const TestData& data = testData[i];
            EXPECT_EQ(data.a, TestData::DefaultValueA);
            EXPECT_EQ(data.b, TestData::DefaultValueB);
            EXPECT_EQ(data.c, TestData::DefaultValueC);
        }
        
        for (size_t i = 0; i < container.GetSize(); ++i)
        {
            EXPECT_EQ(testints[i], i * 10);
        }
        
        for (size_t i = 0; i < container.GetSize(); ++i)
        {
            EXPECT_EQ(testfloats[i], i * 20.0f);
        }

        container.Clear();
        EXPECT_EQ(0, container.GetSize());
    }

    TEST_F(SparseVectorTests, SparseVectorNonTrivialDestructor)
    {
        constexpr size_t Count = 10;

        SparseVector<TestDataWithDestructor> container;
        for (size_t i = 0; i < Count; ++i)
        {
            container.Reserve();
        }
        // Release some elements to call their destructor
        for (size_t i = 0; i < Count / 2; ++i)
        {
            container.Release(i);
        }
        container.Clear(); // TestDataWithDestructor checks in its destructor for double-deletes;

        MultiSparseVector<TestDataWithDestructor, TestData> multiContainer;
        for (size_t i = 0; i < Count; ++i)
        {
            multiContainer.Reserve();
        }
        // Release some elements to call their destructor
        for (size_t i = 0; i < Count / 2; ++i)
        {
            multiContainer.Release(i);
        }
        multiContainer.Clear(); // TestDataWithDestructor checks in its destructor for double-deletes;
    }
}

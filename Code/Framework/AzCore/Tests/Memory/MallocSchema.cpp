/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/MallocSchema.h>

using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    class MallocSchemaTest : public ::testing::Test
    {
    public:
        void RunTests()
        {
            TestAllocationDeallocation();
            TestReallocation();
        }

    private:
        void TestAllocationDeallocation()
        {
            static const AZStd::pair<size_t, size_t> sizeAndAlignments[] =
            {
                { 16, 8 },
                { 15, 1 },
                { 17, 1 },
                { 1024, 16 },
                { 1023, 1 },
                { 1025, 1 },
                { 65536, 16 },
                { 65535, 1 },
                { 65537, 1 }
            };

            AZStd::fixed_vector<void*, AZ_ARRAY_SIZE(sizeAndAlignments)> records;
            size_t totalAllocated = 0;

            // Allocate memory
            for (const auto& sizeAndAlignment : sizeAndAlignments)
            {
                void* p = m_mallocSchema.Allocate(sizeAndAlignment.first, sizeAndAlignment.second, 0);
                EXPECT_NE(p, nullptr);
                EXPECT_EQ(m_mallocSchema.AllocationSize(p), sizeAndAlignment.first);
                records.push_back(p);
                totalAllocated += sizeAndAlignment.first;
                EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), totalAllocated);
            }

            // Deallocate memory
            for (auto record : records)
            {
                totalAllocated -= m_mallocSchema.AllocationSize(record);
                m_mallocSchema.DeAllocate(record);
                EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), totalAllocated);
            }

            EXPECT_EQ(totalAllocated, 0);
        }

        void TestReallocation()
        {
            static const AZStd::pair<size_t, size_t> sizeAndAlignments[] =
            {
                { 16, 8 },
                { 1024, 16 },
                { 32, 8 },
                { 0, 0 }
            };

            void* p = nullptr;
           
            // Keep reallocating the same pointer
            for (const auto& sizeAndAlignment : sizeAndAlignments)
            {
                p = m_mallocSchema.ReAllocate(p, sizeAndAlignment.first, sizeAndAlignment.second);

                if (sizeAndAlignment.first)
                {
                    EXPECT_NE(p, nullptr);
                    EXPECT_EQ(m_mallocSchema.AllocationSize(p), sizeAndAlignment.first);
                    EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), sizeAndAlignment.first);
                }
                else
                {
                    EXPECT_EQ(p, nullptr);
                    EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), 0);
                }
            }
        }

        MallocSchema m_mallocSchema;
    };

    TEST_F(MallocSchemaTest, Test)
    {
        RunTests();
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/AllocatorOverrideShim.h>
#include <AzCore/Memory/MallocSchema.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    namespace
    {
        class AllocatorManagerTestAllocator : public AZ::SystemAllocator
        {
        public:
            AZ_TYPE_INFO(SystemAllocator, "{7E39571D-2E09-416C-BA37-F443C78110FA}")
        };
    }

    class AllocatorManagerTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {

            if (AllocatorInstance<OSAllocator>::IsReady())
            {
                AllocatorInstance<OSAllocator>::Destroy();
            }
        }

        void TearDown() override
        {
            if (!AllocatorInstance<OSAllocator>::IsReady())
            {
                AllocatorInstance<OSAllocator>::Create();
            }
        }

        void RunTests()
        {
            TestAllocatorShimWithDeallocateAfter();
            TestAllocatorShimRemovedAfterFinalization();
            TestAllocatorShimUsedForRealloc();
            TearDownAllocatorManagerTest();
        }

    private:
        static const int TEST_ALLOC_BYTES = 16;

        void SetUpAllocatorManagerTest()
        {
            TearDownAllocatorManagerTest();
            m_manager = &AllocatorManager::Instance();

            EXPECT_EQ(m_manager->GetNumAllocators(), 0);
            AllocatorInstance<SystemAllocator>::Create();
            EXPECT_EQ(m_manager->GetNumAllocators(), 2);  // SystemAllocator creates the OSAllocator if it doesn't exist
            m_systemAllocator = &AllocatorInstance<SystemAllocator>::GetAllocator();
        }

        void TearDownAllocatorManagerTest()
        {
            if (AllocatorInstance<SystemAllocator>::IsReady())
            {
                AllocatorInstance<SystemAllocator>::Destroy();
            }

            if (AllocatorManager::IsReady())
            {
                AllocatorManager::Destroy();
            }

            m_manager = nullptr;
            m_systemAllocator = nullptr;
        }

        void TestAllocatorShimWithDeallocateAfter()
        {
            SetUpAllocatorManagerTest();

            // Should begin with a shim installed. If this fails, check that AZCORE_MEMORY_ENABLE_OVERRIDES is enabled in AllocatorManager.cpp.
            EXPECT_NE(m_systemAllocator->GetAllocationSource(), m_systemAllocator->GetOriginalAllocationSource());

            const int testAllocBytes = TEST_ALLOC_BYTES;
            
            // Allocate from the shim, which should take from the allocator's original source
            void* p = m_systemAllocator->GetAllocationSource()->Allocate(testAllocBytes, 0, 0);
            EXPECT_NE(p, nullptr);
            EXPECT_EQ(m_systemAllocator->GetOriginalAllocationSource()->NumAllocatedBytes(), testAllocBytes);

            // Add the override schema
            m_manager->SetOverrideAllocatorSource(&m_mallocSchema);

            // Allocations should go through malloc schema instead of the allocator's regular schema
            void* q = m_systemAllocator->GetAllocationSource()->Allocate(testAllocBytes, 0, 0);
            EXPECT_NE(q, nullptr);
            EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), testAllocBytes);
            EXPECT_EQ(m_systemAllocator->GetOriginalAllocationSource()->NumAllocatedBytes(), testAllocBytes);

            // Finalize configuration, no more shims should be created after this point
            m_manager->FinalizeConfiguration();

            // Deallocating the original orphaned allocation from the SystemAllocator should remove the shim
            EXPECT_NE(m_systemAllocator->GetAllocationSource(), &m_mallocSchema);
            m_systemAllocator->GetAllocationSource()->DeAllocate(p);
            EXPECT_EQ(m_systemAllocator->GetAllocationSource(), &m_mallocSchema);

            // Clean up
            m_systemAllocator->GetAllocationSource()->DeAllocate(q);
        }

        void TestAllocatorShimRemovedAfterFinalization()
        {
            SetUpAllocatorManagerTest();

            // Should begin with a shim installed
            EXPECT_NE(m_systemAllocator->GetAllocationSource(), m_systemAllocator->GetOriginalAllocationSource());

            // Finalizing the configuration should remove the shim if it was unused
            m_manager->FinalizeConfiguration();
            EXPECT_EQ(m_systemAllocator->GetAllocationSource(), m_systemAllocator->GetOriginalAllocationSource());
        }

        void TestAllocatorShimUsedForRealloc()
        {
            SetUpAllocatorManagerTest();

            // Should begin with a shim installed
            EXPECT_NE(m_systemAllocator->GetAllocationSource(), m_systemAllocator->GetOriginalAllocationSource());

            const int testAllocBytes = TEST_ALLOC_BYTES;
            
            // Allocate from the shim, which should take from the allocator's original source
            void* p = m_systemAllocator->GetAllocationSource()->Allocate(testAllocBytes, 0, 0);
            EXPECT_NE(p, nullptr);
            EXPECT_EQ(m_systemAllocator->GetOriginalAllocationSource()->NumAllocatedBytes(), testAllocBytes);

            // Add the override schema and finalize
            m_manager->SetOverrideAllocatorSource(&m_mallocSchema);
            m_manager->FinalizeConfiguration();

            // Shim should still be present
            EXPECT_NE(m_systemAllocator->GetAllocationSource(), &m_mallocSchema);

            // Reallocation should move allocation from the old source to the new source
            EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), 0);
            void* q = m_systemAllocator->GetAllocationSource()->ReAllocate(p, testAllocBytes * 2, 0);
            EXPECT_NE(p, q);
            EXPECT_EQ(m_mallocSchema.NumAllocatedBytes(), testAllocBytes * 2);
            EXPECT_EQ(m_systemAllocator->GetOriginalAllocationSource()->NumAllocatedBytes(), 0);

            // Reallocation should also have removed the shim as it was no longer necessary
            EXPECT_EQ(m_systemAllocator->GetAllocationSource(), &m_mallocSchema);
        }

        MallocSchema m_mallocSchema;
        AllocatorManager* m_manager = nullptr;
        IAllocator* m_systemAllocator = nullptr;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_MANAGER_TESTS
    TEST_F(AllocatorManagerTests, DISABLED_Test)
#else
    TEST_F(AllocatorManagerTests, Test)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_MANAGER_TESTS
    {
        RunTests();
    }

}

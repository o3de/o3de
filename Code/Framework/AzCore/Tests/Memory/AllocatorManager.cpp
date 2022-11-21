/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    namespace
    {
        class AllocatorManagerTestAllocator : public AZ::SystemAllocator
        {
        public:
            AZ_RTTI(SystemAllocator, "{7E39571D-2E09-416C-BA37-F443C78110FA}", AZ::SystemAllocator)
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
            m_systemAllocator = &AllocatorInstance<SystemAllocator>::Get();
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

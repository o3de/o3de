/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/AssetTrackingTypesImpl.h>
#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

using namespace AZ;

namespace UnitTest
{
    namespace
    {
        struct TestData
        {
        };

        using AssetTree = AZ::Debug::AssetTree<TestData>;
        using AllocationTable = AZ::Debug::AllocationTable<TestData>;

        struct AssetTrackingTestEnvironment
        {
            AssetTrackingTestEnvironment() : m_table(m_mutex)
            {
            }

            AZStd::mutex m_mutex;
            AssetTree m_tree;
            AllocationTable m_table;
            AZStd::unique_ptr<AZ::Debug::AssetTracking> m_assetTracking;
        };
    }

    class AssetTrackingTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::Debug::AssetTrackingAllocator>::Create();
            m_env.reset(new AssetTrackingTestEnvironment);
            m_env->m_assetTracking.reset(aznew AZ::Debug::AssetTracking(&m_env->m_tree, &m_env->m_table));
        }

        void TearDown() override
        {
            m_env.reset();
            AZ::AllocatorInstance<AZ::Debug::AssetTrackingAllocator>::Destroy();
        }

        void RunTests()
        {
            TestAssetScopes();
            TestScopedAllocation();
            TestScopeAttach();
        }

    private:
        void TestAssetScopes()
        {
            const char* debugScopeText;
            {
                AZ_ASSET_NAMED_SCOPE("TestAssetScopes.1");
                {
                    AZ_ASSET_NAMED_SCOPE("TestAssetScopes.2");
                    {
                        AZ_ASSET_NAMED_SCOPE("TestAssetScopes.3");
                        debugScopeText = AZ::Debug::AssetTracking::GetDebugScope();
                        EXPECT_STREQ(debugScopeText, "TestAssetScopes.3\nTestAssetScopes.2\nTestAssetScopes.1\n");
                    }

                    debugScopeText = AZ::Debug::AssetTracking::GetDebugScope();
                    EXPECT_STREQ(debugScopeText, "TestAssetScopes.2\nTestAssetScopes.1\n");
                }


                debugScopeText = AZ::Debug::AssetTracking::GetDebugScope();
                EXPECT_STREQ(debugScopeText, "TestAssetScopes.1\n");
            }
        }

        void TestScopedAllocation()
        {
            void* TEST_POINTER = (void*)0xDEADBEEFull;
            static const size_t TEST_SIZE = 32;

            {
                AZ_ASSET_NAMED_SCOPE("TestScopedAllocation.1");
                AZ::Debug::AssetTreeNodeBase* activeAsset = m_env->m_assetTracking->GetCurrentThreadAsset();
                m_env->m_table.Get().emplace(TEST_POINTER, AllocationTable::RecordType{ activeAsset, (uint32_t)TEST_SIZE, TestData() });
            }

            auto& rootAsset = m_env->m_tree.m_rootAssets;
            auto itr = rootAsset.m_children.find("TestScopedAllocation.1");

            EXPECT_EQ(&rootAsset, &m_env->m_tree.GetRoot());
            ASSERT_NE(itr, rootAsset.m_children.end());
            EXPECT_EQ(itr->second.m_primaryinfo->m_id->m_id, "TestScopedAllocation.1");

            EXPECT_EQ(&itr->second, m_env->m_table.FindAllocation(TEST_POINTER));

            auto allocationRecord = m_env->m_table.Get().find(TEST_POINTER);
            ASSERT_NE(allocationRecord, m_env->m_table.Get().end());
            EXPECT_EQ(allocationRecord->second.m_asset, &itr->second);
            EXPECT_EQ(allocationRecord->second.m_size, TEST_SIZE);
            
            // Test realocation
            void* TEST_REALLOC_POINTER = (void*)0xDEADC0DEull;
            static const size_t TEST_REALLOC_SIZE = 128;

            m_env->m_table.ReallocateAllocation(TEST_POINTER, TEST_REALLOC_POINTER, TEST_REALLOC_SIZE);
            allocationRecord = m_env->m_table.Get().find(TEST_POINTER);
            EXPECT_EQ(allocationRecord, m_env->m_table.Get().end());
            allocationRecord = m_env->m_table.Get().find(TEST_REALLOC_POINTER);
            ASSERT_NE(allocationRecord, m_env->m_table.Get().end());
            EXPECT_EQ(allocationRecord->second.m_asset, &itr->second);
            EXPECT_EQ(allocationRecord->second.m_size, TEST_REALLOC_SIZE);

            // Test resize
            static const size_t TEST_RESIZE_SIZE = 1024;
            m_env->m_table.ResizeAllocation(TEST_REALLOC_POINTER, TEST_RESIZE_SIZE);
            EXPECT_EQ(allocationRecord->second.m_size, TEST_RESIZE_SIZE);
        }

        void TestScopeAttach()
        {
            void* TEST_POINTER = (void*)0xDEADBEEFull;
            static const size_t TEST_SIZE = 32;
            AZ::Debug::AssetTreeNodeBase* originatingAsset;

            {
                AZ_ASSET_ATTACH_TO_SCOPE(TEST_POINTER);
                EXPECT_EQ(m_env->m_assetTracking->GetCurrentThreadAsset(), nullptr);
            }

            {
                AZ_ASSET_NAMED_SCOPE("TestScopeAttach.1");
                {
                    AZ_ASSET_NAMED_SCOPE("TestScopeAttach.2");
                    {
                        originatingAsset = m_env->m_assetTracking->GetCurrentThreadAsset();
                        EXPECT_NE(originatingAsset, nullptr);
                        m_env->m_table.Get().emplace(TEST_POINTER, AllocationTable::RecordType{ originatingAsset, (uint32_t)TEST_SIZE, TestData() });
                    }
                }
            }

            EXPECT_EQ(m_env->m_assetTracking->GetCurrentThreadAsset(), nullptr);

            {
                AZ_ASSET_ATTACH_TO_SCOPE(TEST_POINTER);
                EXPECT_EQ(m_env->m_assetTracking->GetCurrentThreadAsset(), originatingAsset);
            }
        }

        AZStd::unique_ptr<AssetTrackingTestEnvironment> m_env;
    };

    TEST_F(AssetTrackingTests, Test)
    {
        RunTests();
    }
}

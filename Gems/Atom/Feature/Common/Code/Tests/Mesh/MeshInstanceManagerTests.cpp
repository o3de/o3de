/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshInstanceManager.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace
{
}

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    constexpr size_t keyCount = 4;
    AZ::Data::InstanceId modelIdA = AZ::Data::InstanceId::CreateFromAssetId({ AZ::Uuid::CreateRandom(), 0 });
    AZ::Data::InstanceId modelIdB = AZ::Data::InstanceId::CreateFromAssetId({ AZ::Uuid::CreateRandom(), 0 });
    uint32_t testLodIndex = 0;
    uint32_t testMeshIndex = 0;
    RHI::DrawItemSortKey testSortKey = 0;
    AZ::Data::InstanceId materialIdA = AZ::Data::InstanceId::CreateFromAssetId({ AZ::Uuid::CreateRandom(), 0 });
    AZ::Data::InstanceId materialIdB = AZ::Data::InstanceId::CreateFromAssetId({ AZ::Uuid::CreateRandom(), 0 });

    class MeshInstanceManagerTestFixture
        : public LeakDetectionFixture
    {
        void SetUp() override
        {
            // Add the initial instances
            for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
            {
                m_indices[i] = m_meshInstanceManager.AddInstance(m_uniqueKeys[i]);
            }
        }

    public:
        MeshInstanceManager m_meshInstanceManager;

        AZStd::array<MeshInstanceGroupKey, keyCount> m_uniqueKeys{
            MeshInstanceGroupKey{ modelIdA, testLodIndex, testMeshIndex, materialIdA, Uuid::CreateNull(), testSortKey },
            MeshInstanceGroupKey{ modelIdA, testLodIndex, testMeshIndex, materialIdB, Uuid::CreateNull(), testSortKey },
            MeshInstanceGroupKey{ modelIdB, testLodIndex, testMeshIndex, materialIdA, Uuid::CreateNull(), testSortKey },
            MeshInstanceGroupKey{ modelIdB, testLodIndex, testMeshIndex, materialIdB, Uuid::CreateNull(), testSortKey }
        };

        AZStd::array<MeshInstanceManager::InsertResult, keyCount> m_indices{};
    };

    TEST_F(MeshInstanceManagerTestFixture, AddInstance)
    {
        // Each key was unique, so each index should also be unique
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            for (size_t j = i + 1; j < m_indices.size(); ++j)
            {
                EXPECT_NE(m_indices[i].m_handle, m_indices[j].m_handle);
            }

            // None of these have been intiailized before, so they should not all have an instance count of 1 after adding
            EXPECT_EQ(m_indices[i].m_instanceCount, 1);
        }
    }

    TEST_F(MeshInstanceManagerTestFixture, RemoveInstanceByKey)
    {
        // Remove all of the entries
        for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);
        }

        // All objects were removed, the size of the data vector should be 0
        EXPECT_EQ(m_meshInstanceManager.GetInstanceGroupCount(), 0);
    }

    TEST_F(MeshInstanceManagerTestFixture, RemoveInstanceByIndex)
    {
        // Remove all of the entries
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_indices[i].m_handle);
        }

        // All objects were removed, the size of the data vector should be 0
        EXPECT_EQ(m_meshInstanceManager.GetInstanceGroupCount(), 0);
    }

    TEST_F(MeshInstanceManagerTestFixture, IncreaseRefCount)
    {
        // Increase the refcount of one of the keys
        size_t refCountIncreaseIndex = 2;
        MeshInstanceManager::InsertResult instanceIndex = m_meshInstanceManager.AddInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // We should get back the same instace index that was given originally
        EXPECT_EQ(instanceIndex.m_handle, m_indices[refCountIncreaseIndex].m_handle);
        // It was not inserted, since it already existed
        EXPECT_GT(instanceIndex.m_instanceCount, 1);
        
        // Remove all of the entries
        for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);
        }

        // The entry at refCountIncreaseIndex should still exist. The rest should not.
        // Try adding them again to see if they still exist
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            MeshInstanceManager::InsertResult existenceCheck = m_meshInstanceManager.AddInstance(m_uniqueKeys[i]);
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);

            if (i == refCountIncreaseIndex)
            {
                // Expect that it already existed
                EXPECT_GT(existenceCheck.m_instanceCount, 1);
            }
            else
            {
                // Expect that it was inserted
                EXPECT_EQ(existenceCheck.m_instanceCount, 1);
            }
        }

        // If we remove the entry one more time, it should no longer exist
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // Check that it was previously removed by adding it again and verifying the instance count
        MeshInstanceManager::InsertResult existenceCheck = m_meshInstanceManager.AddInstance(m_uniqueKeys[refCountIncreaseIndex]);
        EXPECT_EQ(existenceCheck.m_instanceCount, 1);

        // You should be able to remove it again
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // It should error if you try to remove it again, since it no longer exits
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

} // namespace UnitTest

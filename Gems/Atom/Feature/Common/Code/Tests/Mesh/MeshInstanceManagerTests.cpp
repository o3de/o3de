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
    constexpr size_t keyCount = 4;
    AZ::Data::InstanceId modelIdA = AZ::Data::InstanceId{ AZ::Uuid::CreateRandom(), 0 };
    AZ::Data::InstanceId modelIdB = AZ::Data::InstanceId{ AZ::Uuid::CreateRandom(), 0 };
    uint32_t lodIndex = 0;
    uint32_t meshIndex = 0;
    AZ::Data::InstanceId materialIdA = AZ::Data::InstanceId{ AZ::Uuid::CreateRandom(), 0 };
    AZ::Data::InstanceId materialIdB = AZ::Data::InstanceId{ AZ::Uuid::CreateRandom(), 0 };
}

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;


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

        AZStd::array<MeshInstanceKey, keyCount> m_uniqueKeys{ MeshInstanceKey{
                                                                  modelIdA, lodIndex, meshIndex, materialIdA, Uuid::CreateNull(), 0, 0 },
            MeshInstanceKey{ modelIdA, lodIndex, meshIndex, materialIdB, Uuid::CreateNull(), 0, 0 },
            MeshInstanceKey{ modelIdB, lodIndex, meshIndex, materialIdA, Uuid::CreateNull(), 0, 0 },
            MeshInstanceKey{ modelIdB, lodIndex, meshIndex, materialIdB, Uuid::CreateNull(), 0, 0 }
        };

        AZStd::array<InsertResult, keyCount> m_indices{};
    };

    TEST_F(MeshInstanceManagerTestFixture, AddInstance)
    {
        // Each key was unique, so each index should also be unique
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            for (size_t j = i + 1; j < m_indices.size(); ++j)
            {
                EXPECT_NE(m_indices[i].m_index, m_indices[j].m_index);
            }

            // None of these have been intiailized yet, so they should not have been found
            EXPECT_TRUE(m_indices[i].m_wasInserted);
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
        EXPECT_EQ(m_meshInstanceManager.GetItemCount(), 0);
    }

    TEST_F(MeshInstanceManagerTestFixture, RemoveInstanceByIndex)
    {
        // Remove all of the entries
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_indices[i].m_index);
        }

        // All objects were removed, the size of the data vector should be 0
        EXPECT_EQ(m_meshInstanceManager.GetItemCount(), 0);
    }

    TEST_F(MeshInstanceManagerTestFixture, IncreaseRefCount)
    {
        // Increase the refcount of one of the keys
        size_t refCountIncreaseIndex = 2;
        InsertResult instanceIndex = m_meshInstanceManager.AddInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // We should get back the same instace index that was given originally
        EXPECT_EQ(instanceIndex.m_index, m_indices[refCountIncreaseIndex].m_index);
        // It was not inserted, since it already existed
        EXPECT_FALSE(instanceIndex.m_wasInserted);
        
        // Remove all of the entries
        for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);
        }

        // The entry at instanceIndex should still exist. The rest should not.
        // Try adding them again to see if they still exist
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            InsertResult existenceCheck = m_meshInstanceManager.AddInstance(m_uniqueKeys[i]);
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);

            if (i == refCountIncreaseIndex)
            {
                EXPECT_FALSE(existenceCheck.m_wasInserted);
            }
            else
            {
                EXPECT_TRUE(existenceCheck.m_wasInserted);
            }
        }

        // If we remove the entry one more time, it should no longer exist
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // Check by adding it again
        InsertResult existenceCheck = m_meshInstanceManager.AddInstance(m_uniqueKeys[refCountIncreaseIndex]);
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);
        EXPECT_TRUE(existenceCheck.m_wasInserted);
    }

} // namespace UnitTest

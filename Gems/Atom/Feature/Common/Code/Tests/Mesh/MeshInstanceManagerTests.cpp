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
    AZ::Data::AssetId modelIdA = AZ::Data::AssetId{ AZ::Uuid::CreateRandom(), 0 };
    AZ::Data::AssetId modelIdB = AZ::Data::AssetId{ AZ::Uuid::CreateRandom(), 0 };
    uint32_t lodIndex = 0;
    uint32_t meshIndex = 0;
    AZ::Data::AssetId materialIdA = AZ::Data::AssetId{ AZ::Uuid::CreateRandom(), 0 };
    AZ::Data::AssetId materialIdB = AZ::Data::AssetId{ AZ::Uuid::CreateRandom(), 0 };
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

        AZStd::array<MeshInstanceKey, keyCount> m_uniqueKeys{ MeshInstanceKey{ modelIdA, lodIndex, meshIndex, materialIdA },
                                                      MeshInstanceKey{ modelIdA, lodIndex, meshIndex, materialIdB },
                                                      MeshInstanceKey{ modelIdB, lodIndex, meshIndex, materialIdA },
                                                        MeshInstanceKey{ modelIdB, lodIndex, meshIndex, materialIdB } };

        AZStd::array<uint32_t, keyCount> m_indices{};
    };

    TEST_F(MeshInstanceManagerTestFixture, AddInstance)
    {
        // Each key was unique, so each index should also be unique
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            for (size_t j = i + 1; j < m_indices.size(); ++j)
            {
                EXPECT_NE(m_indices[i], m_indices[j]);
            }
        }

        // None of these have been intiailized yet, so they should all exist in the new entry list
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            EXPECT_NE(
                AZStd::find(
                    m_meshInstanceManager.GetUninitializedInstances().begin(),
                    m_meshInstanceManager.GetUninitializedInstances().end(),
                    m_indices[i]),
                m_meshInstanceManager.GetUninitializedInstances().end());
        }
    }

    TEST_F(MeshInstanceManagerTestFixture, RemoveInstance)
    {
        // Remove all of the entries
        for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);
        }

        // All objects were removed, the size of the data vector should be 0
        EXPECT_EQ(m_meshInstanceManager.GetDataList().size(), 0);

        // The objects were removed so they should not be in the new entry list
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            EXPECT_EQ(
                AZStd::find(
                    m_meshInstanceManager.GetUninitializedInstances().begin(),
                    m_meshInstanceManager.GetUninitializedInstances().end(),
                    m_indices[i]),
                m_meshInstanceManager.GetUninitializedInstances().end());
        }
    }

    TEST_F(MeshInstanceManagerTestFixture, IncreaseRefCount)
    {
        // Increase the refcount of one of the keys
        size_t refCountIncreaseIndex = 2;
        uint32_t instanceIndex = m_meshInstanceManager.AddInstance(m_uniqueKeys[refCountIncreaseIndex]);

        // We should get back the same instace index that was given originally
        EXPECT_EQ(instanceIndex, m_indices[refCountIncreaseIndex]);
        
        // Remove all of the entries
        for (size_t i = 0; i < m_uniqueKeys.size(); ++i)
        {
            m_meshInstanceManager.RemoveInstance(m_uniqueKeys[i]);
        }

        // The entry at instanceIndex should still exist. The rest should not.
        // Use the uninitialized instances list to verify
        for (size_t i = 0; i < m_indices.size(); ++i)
        {
            auto iter = AZStd::find(
                m_meshInstanceManager.GetUninitializedInstances().begin(),
                m_meshInstanceManager.GetUninitializedInstances().end(),
                m_indices[i]);

            if (i == refCountIncreaseIndex)
            {
                EXPECT_NE(iter, m_meshInstanceManager.GetUninitializedInstances().end());
            }
            else
            {
                EXPECT_EQ(iter, m_meshInstanceManager.GetUninitializedInstances().end());
            }
        }

        // If we remove the entry one more time, it should no longer exist in the new entry list
        m_meshInstanceManager.RemoveInstance(m_uniqueKeys[refCountIncreaseIndex]);
        auto iter = AZStd::find(
            m_meshInstanceManager.GetUninitializedInstances().begin(),
            m_meshInstanceManager.GetUninitializedInstances().end(),
            m_indices[refCountIncreaseIndex]);
        EXPECT_EQ(iter, m_meshInstanceManager.GetUninitializedInstances().end());
    }

} // namespace UnitTest

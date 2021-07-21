/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/SpawnableSortEntitiesTestFixture.h>

#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Prefab/Spawnable/SpawnableUtils.h>

namespace UnitTest
{
    void SpawnableSortEntitiesTestFixture::TearDownEditorFixtureImpl()
    {
        for (AZ::Entity* entity : m_sorted)
        {
            delete entity;
        }

        m_unsorted.clear();
        m_sorted.clear();
        m_expectedEntities.clear();
        m_actualEntities.clear();
    }

    void SpawnableSortEntitiesTestFixture::AddEntity(AZ::EntityId id, AZ::EntityId parentId, bool expectedInSorted)
    {
        auto* newEntity = aznew AZ::Entity(id);
        newEntity->CreateComponent<AzFramework::TransformComponent>()->SetParent(parentId);

        m_unsorted.push_back(newEntity);
        if (expectedInSorted)
        {
            m_expectedEntities.insert(newEntity);
        }
    }

    void SpawnableSortEntitiesTestFixture::AddEntity(AZ::Entity* entity, bool expectedInSorted)
    {
        m_unsorted.push_back(entity);
        if (expectedInSorted)
        {
            m_expectedEntities.insert(entity);
        }
    }

    void SpawnableSortEntitiesTestFixture::ConvertEntitiesToSpawnable(const AZStd::vector<AZ::Entity*>& rawEntities)
    {
        auto& entities = m_spawnable.GetEntities();
        entities.clear();
        entities.reserve(rawEntities.size());

        AZStd::for_each(rawEntities.begin(), rawEntities.end(),
            [&entities](auto& rawEntity)
            {
                entities.emplace_back(AZStd::unique_ptr<AZ::Entity>(rawEntity));
            }
        );
    }

    void SpawnableSortEntitiesTestFixture::ConvertoSpawnableToEntities(AZStd::vector<AZ::Entity*>& rawEntities)
    {
        auto& entities = m_spawnable.GetEntities();
        rawEntities.clear();
        rawEntities.reserve(entities.size());

        AZStd::for_each(entities.begin(), entities.end(),
            [this, &rawEntities](auto& entity)
            {
                auto* rawEntity = entity.release();
                m_actualEntities.insert(rawEntity);
                rawEntities.emplace_back(rawEntity);
            }
        );
    }

    void SpawnableSortEntitiesTestFixture::SortAndSanityCheck()
    {
        ConvertEntitiesToSpawnable(m_unsorted);
        AzToolsFramework::Prefab::SpawnableUtils::SortEntitiesByTransformHierarchy(m_spawnable);
        ConvertoSpawnableToEntities(m_sorted);

        // sanity check that all entries are still there
        EXPECT_EQ(m_expectedEntities, m_actualEntities);
    }

    bool SpawnableSortEntitiesTestFixture::IsChildAfterParent(AZ::EntityId childId, AZ::EntityId parentId)
    {
        int parentIndex = -1;
        int childIndex = -1;
        for (int i = 0; i < m_sorted.size(); ++i)
        {
            if (m_sorted[i] && (m_sorted[i]->GetId() == parentId) && (parentIndex == -1))
            {
                parentIndex = i;
            }
            if (m_sorted[i] && (m_sorted[i]->GetId() == childId) && (childIndex == -1))
            {
                childIndex = i;
            }
        }

        EXPECT_NE(childIndex, -1);
        EXPECT_NE(parentIndex, -1);
        return childIndex > parentIndex;
    }
}

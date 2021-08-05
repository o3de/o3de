/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestFixture.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace UnitTest
{
    using SpawnableCreateTest = PrefabTestFixture;

    TEST_F(SpawnableCreateTest, SpawnableCreate_NoNestingPrefabDom_CreateSucceeds)
    {
        AZStd::vector<AZ::Entity*> entitiesCreated;
        AZStd::unordered_set<AZStd::string> expectedEntityNameSet;
        const int normalEntityCount = 3;
        for (int i = 0; i < normalEntityCount; i++)
        {
            entitiesCreated.push_back(CreateEntity(AZStd::string::format("Entity_%i", i).c_str()));
            expectedEntityNameSet.insert(entitiesCreated.back()->GetName());
        }

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> instance(
            m_prefabSystemComponent->CreatePrefab(entitiesCreated, {}, "test/path"));
        ASSERT_TRUE(instance);

        ASSERT_TRUE(instance->HasContainerEntity());
        auto& containerEntity = instance->GetContainerEntity()->get();
        expectedEntityNameSet.insert(containerEntity.GetName());

        //Create Spawnable
        auto& prefabDom = m_prefabSystemComponent->FindTemplateDom(instance->GetTemplateId());
        AzFramework::Spawnable spawnable;
        AzToolsFramework::Prefab::SpawnableUtils::CreateSpawnable(spawnable, prefabDom);

        EXPECT_EQ(spawnable.GetEntities().size() - 1, normalEntityCount); // 1 for container entity
        const auto& spawnableEntities = spawnable.GetEntities();
        AZStd::unordered_set<AZStd::string> actualEntityNameSet;

        for (const auto& spawnableEntity : spawnableEntities)
        {
            actualEntityNameSet.insert(spawnableEntity->GetName());
        }

        EXPECT_EQ(expectedEntityNameSet, actualEntityNameSet);
    }

    TEST_F(SpawnableCreateTest, SpawnableCreate_TripleNestingPrefabDom_CreateSucceeds)
    {
        AZStd::vector<AZ::Entity*> entitiesCreated;
        AZStd::unordered_set<AZStd::string> expectedEntityNameSet;
        const int normalEntityCount = 3;
        for (int i = 0; i < normalEntityCount; i++)
        {
            entitiesCreated.push_back(CreateEntity(AZStd::string::format("Entity_%i", i).c_str()));
            expectedEntityNameSet.insert(entitiesCreated.back()->GetName());
        }

        // Build a 3 level deep nested Template
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> firstInstance(
            m_prefabSystemComponent->CreatePrefab({ entitiesCreated[0] }, {}, "test/path1"));
        ASSERT_TRUE(firstInstance);

        ASSERT_TRUE(firstInstance->HasContainerEntity());
        expectedEntityNameSet.insert(firstInstance->GetContainerEntity()->get().GetName());

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> secondInstance(
            m_prefabSystemComponent->CreatePrefab({ entitiesCreated[1] }, MakeInstanceList(AZStd::move(firstInstance)), "test/path2"));
        ASSERT_TRUE(secondInstance);

        ASSERT_TRUE(secondInstance->HasContainerEntity());
        expectedEntityNameSet.insert(secondInstance->GetContainerEntity()->get().GetName());

        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> thirdInstance(
            m_prefabSystemComponent->CreatePrefab({ entitiesCreated[2] }, MakeInstanceList(AZStd::move(secondInstance)), "test/path3"));
        ASSERT_TRUE(thirdInstance);

        ASSERT_TRUE(thirdInstance->HasContainerEntity());
        expectedEntityNameSet.insert(thirdInstance->GetContainerEntity()->get().GetName());

        //Create Spawnable
        auto& prefabDom = m_prefabSystemComponent->FindTemplateDom(thirdInstance->GetTemplateId());
        AzFramework::Spawnable spawnable;
        AzToolsFramework::Prefab::SpawnableUtils::CreateSpawnable(spawnable, prefabDom);

        EXPECT_EQ(spawnable.GetEntities().size(), normalEntityCount + 3); // +1 for each container entity
        const auto& spawnableEntities = spawnable.GetEntities();
        AZStd::unordered_set<AZStd::string> actualEntityNameSet;

        for (const auto& spawnableEntity : spawnableEntities)
        {
            actualEntityNameSet.insert(spawnableEntity->GetName());
        }

        EXPECT_EQ(actualEntityNameSet, expectedEntityNameSet);
    }
}

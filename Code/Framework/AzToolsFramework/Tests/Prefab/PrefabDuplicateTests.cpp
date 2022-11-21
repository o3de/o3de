/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabDuplicateTest = PrefabTestFixture;

    TEST_F(PrefabDuplicateTest, PrefabDuplicate_DuplicateSingleEntitySucceeds)
    {
        AZStd::string entityName("Same Name");
        AZ::Entity* entity1 = CreateEntity(entityName.c_str());
        entity1->Deactivate();
        entity1->CreateComponent<PrefabTestComponent>();
        entity1->Activate();

        AddRequiredEditorComponents({ entity1->GetId() });

        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab(
            { entity1 },
            {},
            PrefabMockFilePath);

        // We've created a prefab with a single Entity, so there should only be one EntityAlias in our instance
        EXPECT_EQ(newInstance->GetEntityAliases().size(), 1);

        // Duplicate the Entity and trigger the UpdateTemplateInstancesInQueue so the changes get propagated
        m_prefabPublicInterface->DuplicateEntitiesInInstance(AzToolsFramework::EntityIdList{ entity1->GetId() });
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // We duplicated a single Entity, so there should now be two EntityAliases
        EXPECT_EQ(newInstance->GetEntityAliases().size(), 2);

        newInstance->GetConstEntities([&](const AZ::Entity& entity)
        {
            // Both of the entities should have the same name
            EXPECT_EQ(entity.GetName(), entityName);

            // Both of the entities should have the PrefabTestComponent we added
            auto testComponent = entity.FindComponent<PrefabTestComponent>();
            EXPECT_NE(nullptr, testComponent);

            return true;
        });
    }

    TEST_F(PrefabDuplicateTest, PrefabDuplicate_DuplicateMultipleEntitiesAndFixesReferences)
    {
        AZ::Entity* parentEntity = CreateEntity("Parent Entity");

        AZ::Entity* childEntity = CreateEntity("Child Entity");
        childEntity->Deactivate();
        auto newComponent = childEntity->CreateComponent<PrefabTestComponent>();
        childEntity->Activate();

        // Set the EntityId reference property on our PrefabTestComponent so we can
        // verify that arbitrary EntityId's are fixed up properly
        newComponent->m_entityIdProperty = parentEntity->GetId();

        AddRequiredEditorComponents({ parentEntity->GetId(), childEntity->GetId() });

        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->CreatePrefab(
            { parentEntity, childEntity },
            {},
            PrefabMockFilePath);

        // We've created a prefab with two entities, so there should be two EntityAliases in our instance
        EXPECT_EQ(newInstance->GetEntityAliases().size(), 2);

        // Duplicate the entities and trigger the UpdateTemplateInstancesInQueue so the changes get propagated
        m_prefabPublicInterface->DuplicateEntitiesInInstance(AzToolsFramework::EntityIdList{ parentEntity->GetId(), childEntity->GetId() });
        m_instanceUpdateExecutorInterface->UpdateTemplateInstancesInQueue();

        // We duplicated two entities, so there should now be four EntityAliases
        EXPECT_EQ(newInstance->GetEntityAliases().size(), 4);

        AzToolsFramework::EntityIdList parentEntityIds;
        newInstance->GetConstEntities([&](const AZ::Entity& entity)
        {
            // Gather the parent EntityIds by tracking which entities don't have a PrefabTestComponent
            auto testComponent = entity.FindComponent<PrefabTestComponent>();
            if (!testComponent)
            {
                parentEntityIds.push_back(entity.GetId());
            }

            return true;
        });

        // There should only be two parents
        EXPECT_EQ(parentEntityIds.size(), 2);

        // Verify that the EntityId reference on the PrefabTestComponent on the children correspond
        // to unique entities, which will verify that the EntityIds are fixed up on duplicate
        newInstance->GetConstEntities([&](const AZ::Entity& entity)
        {
            // Only the child entities have a PrefabTestComponent
            auto testComponent = entity.FindComponent<PrefabTestComponent>();
            if (testComponent)
            {
                auto it = AZStd::find(parentEntityIds.begin(), parentEntityIds.end(), testComponent->m_entityIdProperty);
                EXPECT_NE(it, parentEntityIds.end());

                // Erase when we find it so that the matches will be unique
                parentEntityIds.erase(it);
            }

            return true;
        });

        // Verify we matched each of the parent EntityIds
        EXPECT_EQ(parentEntityIds.size(), 0);
    }
}

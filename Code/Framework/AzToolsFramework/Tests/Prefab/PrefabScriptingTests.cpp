/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Entity/EditorEntityUtilityComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabScriptingTest = PrefabTestFixture;

    TemplateId g_globalTemplateId = {};
    AZStd::string g_globalPrefabString = "";

    TEST_F(PrefabScriptingTest, PrefabScripting_CreatePrefab)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        behaviorContext->Property("g_globalTemplateId", BehaviorValueProperty(&g_globalTemplateId));

        g_globalTemplateId = TemplateId{};
        
        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EditorEntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            entities:push_back(my_id)
            g_globalTemplateId = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab", true)
            )LUA");

        EXPECT_NE(g_globalTemplateId, TemplateId{});

        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);

        TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(g_globalTemplateId);

        EXPECT_TRUE(templateRef);
    }

    TEST_F(PrefabScriptingTest, PrefabScripting_SaveToString)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        behaviorContext->Property("g_globalPrefabString", BehaviorValueProperty(&g_globalPrefabString));
        
        g_globalPrefabString = "";

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EditorEntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            entities:push_back(my_id)
            template_id = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab", true)
            my_result = PrefabLoaderScriptingBus.Broadcast.SaveTemplateToString(template_id)

            if my_result:IsSuccess() then
                g_globalPrefabString = my_result:GetValue()
            end
            )LUA");
        
        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        prefabSystemComponentInterface->RemoveAllTemplates();

        EXPECT_STRNE(g_globalPrefabString.c_str(), "");
        TemplateId templateFromString = AZ::Interface<PrefabLoaderInterface>::Get()->LoadTemplateFromString(g_globalPrefabString);

        EXPECT_NE(templateFromString, InvalidTemplateId);

        // Create another entity for comparison purposes
        AZ::EntityId entityId;
        AzToolsFramework::EditorEntityUtilityBus::BroadcastResult(
            entityId, &AzToolsFramework::EditorEntityUtilityBus::Events::CreateEditorReadyEntity, "test");

        AZ::Entity* testEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);

        // Instantiate the prefab we saved
        AZStd::unique_ptr<Instance> instance = prefabSystemComponentInterface->InstantiatePrefab(templateFromString);

        EXPECT_NE(instance, nullptr);

        AZStd::vector<const AZ::Entity*> loadedEntities;

        // Get the entities from the instance
        instance->GetConstEntities(
            [&loadedEntities](const AZ::Entity& entity)
            {
                loadedEntities.push_back(&entity);
                return true;
            });

        // Make sure the instance has an entity with the same number of components as our test entity
        EXPECT_EQ(loadedEntities.size(), 1);
        EXPECT_EQ(loadedEntities[0]->GetComponents().size(), testEntity->GetComponents().size());

        g_globalPrefabString.set_capacity(0); // Free all memory
    }
    
}

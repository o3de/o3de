/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Entity/EntityUtilityComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    TemplateId g_globalTemplateId = {};
    AZStd::string g_globalPrefabString = "";

    class PrefabScriptingTest : public PrefabTestFixture
    {
        void InitProperties() const
        {
            AZ::ComponentApplicationRequests* componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();

            ASSERT_NE(componentApplicationRequests, nullptr);

            auto behaviorContext = componentApplicationRequests->GetBehaviorContext();

            ASSERT_NE(behaviorContext, nullptr);

            behaviorContext->Property("g_globalTemplateId", BehaviorValueProperty(&g_globalTemplateId));
            behaviorContext->Property("g_globalPrefabString", BehaviorValueProperty(&g_globalPrefabString));

            g_globalTemplateId = TemplateId{};
            g_globalPrefabString = AZStd::string{};
        }

        void SetUpEditorFixtureImpl() override
        {
            InitProperties();
        }

        void TearDownEditorFixtureImpl() override
        {
            g_globalPrefabString.set_capacity(0); // Free all memory
        }
    };

    TEST_F(PrefabScriptingTest, CreatePrefabTemplate_GeneratesComponentsWithTypeNamesAsSerializedIdentifiers)
    {
        AZ::EntityId entityId;
        AzToolsFramework::EntityUtilityBus::BroadcastResult(entityId, &AzToolsFramework::EntityUtilityBus::Events::CreateEditorReadyEntity, "test");
        TemplateId templateId1;
        PrefabSystemScriptingBus::BroadcastResult(templateId1, &PrefabSystemScriptingBus::Events::CreatePrefabTemplate, AZStd::vector{ entityId }, "test.prefab");

        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        auto instance1 = prefabSystemComponentInterface->InstantiatePrefab(templateId1);

        // Clear all templates to reset the system
        prefabSystemComponentInterface->RemoveAllTemplates();

        TemplateId templateId2;
        PrefabSystemScriptingBus::BroadcastResult(templateId2, &PrefabSystemScriptingBus::Events::CreatePrefabTemplate, AZStd::vector{ entityId }, "test.prefab");

        auto instance2 = prefabSystemComponentInterface->InstantiatePrefab(templateId2);

        auto referenceWrapper1 = instance1->GetContainerEntity();
        auto referenceWrapper2 = instance2->GetContainerEntity();

        ASSERT_TRUE(referenceWrapper1);
        ASSERT_TRUE(referenceWrapper2);

        auto transformComponent1 = referenceWrapper1->get().FindComponent<AzToolsFramework::Components::TransformComponent>();
        auto transformComponent2 = referenceWrapper2->get().FindComponent<AzToolsFramework::Components::TransformComponent>();

        EXPECT_EQ(transformComponent1->GetSerializedIdentifier(), transformComponent1->RTTI_GetTypeName());
        EXPECT_EQ(transformComponent2->GetSerializedIdentifier(), transformComponent1->RTTI_GetTypeName());
    }

    TEST_F(PrefabScriptingTest, PrefabScripting_CreatePrefab)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            entities:push_back(my_id)
            g_globalTemplateId = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab")
            )LUA");

        EXPECT_NE(g_globalTemplateId, TemplateId{});

        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);

        TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(g_globalTemplateId);

        EXPECT_TRUE(templateRef);
    }

    TEST_F(PrefabScriptingTest, PrefabScripting_CreatePrefab_NoEntities)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            g_globalTemplateId = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab")
            )LUA");

        EXPECT_NE(g_globalTemplateId, TemplateId{});

        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);

        TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(g_globalTemplateId);

        EXPECT_TRUE(templateRef);
    }

    TEST_F(PrefabScriptingTest, PrefabScripting_CreatePrefab_NoPath)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        AZ_TEST_START_TRACE_SUPPRESSION;
        sc.Execute(R"LUA(
            my_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            template_id = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "")
            )LUA");
        /*
        error: PrefabSystemComponent::CreateTemplateFromInstance - Attempted to create a prefab template from an instance without a source file path. Unable to proceed.
        error: Failed to create a Template associated with file path  during CreatePrefab.
        error: Failed to create prefab
         */
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);
    }

    TEST_F(PrefabScriptingTest, PrefabScripting_SaveToString)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId()
            entities:push_back(my_id)
            template_id = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab")
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
        AzToolsFramework::EntityUtilityBus::BroadcastResult(
            entityId, &AzToolsFramework::EntityUtilityBus::Events::CreateEditorReadyEntity, "test");

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

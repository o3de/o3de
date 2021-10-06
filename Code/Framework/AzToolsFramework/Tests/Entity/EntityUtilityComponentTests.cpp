/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/BehaviorEntity.h>
#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Entity/EntityUtilityComponent.h>
#include <ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    // Global variables for communicating between Lua test code and C++
    AZ::EntityId g_globalEntityId = AZ::EntityId{};
    AZStd::string g_globalString = "";
    AzFramework::BehaviorComponentId g_globalComponentId = {};
    AZStd::vector<AzToolsFramework::ComponentDetails> g_globalComponentDetails = {};
    bool g_globalBool = false;

    class EntityUtilityComponentTests
        : public ToolsApplicationFixture
    {
        void InitProperties()
        {
            AZ::ComponentApplicationRequests* componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();

            ASSERT_NE(componentApplicationRequests, nullptr);

            auto behaviorContext = componentApplicationRequests->GetBehaviorContext();

            ASSERT_NE(behaviorContext, nullptr);

            behaviorContext->Property("g_globalEntityId", BehaviorValueProperty(&g_globalEntityId));
            behaviorContext->Property("g_globalString", BehaviorValueProperty(&g_globalString));
            behaviorContext->Property("g_globalComponentId", BehaviorValueProperty(&g_globalComponentId));
            behaviorContext->Property("g_globalBool", BehaviorValueProperty(&g_globalBool));
            behaviorContext->Property("g_globalComponentDetails", BehaviorValueProperty(&g_globalComponentDetails));

            g_globalEntityId = AZ::EntityId{};
            g_globalString = AZStd::string{};
            g_globalComponentId = AzFramework::BehaviorComponentId{};
            g_globalBool = false;
            g_globalComponentDetails = AZStd::vector<AzToolsFramework::ComponentDetails>{};
        }

        void SetUpEditorFixtureImpl() override
        {
            InitProperties();
        }
        
        void TearDownEditorFixtureImpl() override
        {
            g_globalString.set_capacity(0); // Free all memory
            g_globalComponentDetails.set_capacity(0);
        }
    };

    TEST_F(EntityUtilityComponentTests, CreateEntity)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();
        
        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalEntityId = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            my_entity = Entity(g_globalEntityId)
            g_globalString = my_entity:GetName()
            )LUA");

        EXPECT_NE(g_globalEntityId, AZ::EntityId{});
        EXPECT_STREQ(g_globalString.c_str(), "test");

        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        ASSERT_NE(entity, nullptr);

        // Test cleaning up, make sure the entity is destroyed
        AzToolsFramework::EntityUtilityBus::Broadcast(&AzToolsFramework::EntityUtilityBus::Events::ResetEntityContext);

        entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        ASSERT_EQ(entity, nullptr);
    }

    TEST_F(EntityUtilityComponentTests, CreateEntityEmptyName)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalEntityId = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("")
            )LUA");

        EXPECT_NE(g_globalEntityId, AZ::EntityId{});

        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        ASSERT_NE(entity, nullptr);
    }

    TEST_F(EntityUtilityComponentTests, FindComponent)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();
        
        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            ent_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            g_globalComponentId = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(ent_id, "27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0 TransformComponent")
            )LUA");
        
        EXPECT_TRUE(g_globalComponentId.IsValid());
    }

    TEST_F(EntityUtilityComponentTests, InvalidComponentName)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();
        
        sc.BindTo(behaviorContext);
        AZ_TEST_START_TRACE_SUPPRESSION;
        sc.Execute(R"LUA(
            ent_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            g_globalComponentId = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(ent_id, "ThisIsNotAComponent-Error")
            )LUA");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(g_globalComponentId.IsValid());
    }

    TEST_F(EntityUtilityComponentTests, InvalidComponentId)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();
        
        sc.BindTo(behaviorContext);
        AZ_TEST_START_TRACE_SUPPRESSION;
        sc.Execute(R"LUA(
            ent_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            g_globalComponentId = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(ent_id, "{1234-hello-world-this-is-not-an-id}")
            )LUA");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Should get 1 error stating the type id is not valid
        EXPECT_FALSE(g_globalComponentId.IsValid());
    }

    TEST_F(EntityUtilityComponentTests, CreateComponent)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();
        
        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            ent_id = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            g_globalComponentId = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(ent_id, "ScriptEditorComponent")
            )LUA");

        EXPECT_TRUE(g_globalComponentId.IsValid());
    }

    TEST_F(EntityUtilityComponentTests, UpdateComponent)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalEntityId = EntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            comp_id = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(g_globalEntityId, "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent")
            json_update = [[
            {
                "Transform Data": { "Rotate": [0.0, 0.1, 180.0] }
            }
            ]]
            g_globalBool = EntityUtilityBus.Broadcast.UpdateComponentForEntity(g_globalEntityId, comp_id, json_update);
            )LUA");

        EXPECT_TRUE(g_globalBool);
        EXPECT_NE(g_globalEntityId, AZ::EntityId(AZ::EntityId::InvalidEntityId));

        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        auto* transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();

        ASSERT_NE(transformComponent, nullptr);

        AZ::Vector3 localRotation = transformComponent->GetLocalRotationQuaternion().GetEulerDegrees();

        EXPECT_EQ(localRotation, AZ::Vector3(.0f, 0.1f, 180.0f));
    }

    TEST_F(EntityUtilityComponentTests, GetComponentJson)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalString = EntityUtilityBus.Broadcast.GetComponentDefaultJson("ScriptEditorComponent")
            )LUA");

        EXPECT_STRNE(g_globalString.c_str(), "");
    }

    TEST_F(EntityUtilityComponentTests, GetComponentJsonDoesNotExist)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        AZ_TEST_START_TRACE_SUPPRESSION;
        sc.Execute(R"LUA(
            g_globalString = EntityUtilityBus.Broadcast.GetComponentDefaultJson("404")
            )LUA");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // 1 error: Failed to find component id for type name 404

        EXPECT_STREQ(g_globalString.c_str(), "");
    }

    TEST_F(EntityUtilityComponentTests, SearchComponents)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalComponentDetails = EntityUtilityBus.Broadcast.FindMatchingComponents("Transform*")
            )LUA");

        // There should be 2 transform components
        EXPECT_EQ(g_globalComponentDetails.size(), 2);
    }

    TEST_F(EntityUtilityComponentTests, SearchComponentsNotFound)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalComponentDetails = EntityUtilityBus.Broadcast.FindMatchingComponents("404")
            )LUA");
        
        EXPECT_EQ(g_globalComponentDetails.size(), 0);
    }
}

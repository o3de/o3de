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
#include <ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    AZ::EntityId g_globalEntityId = AZ::EntityId{};
    AZStd::string g_globalEntityName = "";
    AzFramework::BehaviorComponentId g_globalComponentId = {};
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
            behaviorContext->Property("g_globalEntityName", BehaviorValueProperty(&g_globalEntityName));
            behaviorContext->Property("g_globalComponentId", BehaviorValueProperty(&g_globalComponentId));
            behaviorContext->Property("g_globalBool", BehaviorValueProperty(&g_globalBool));

            g_globalEntityId = AZ::EntityId{};
            g_globalEntityName = AZStd::string{};
            g_globalComponentId = AzFramework::BehaviorComponentId{};
            g_globalBool = false;
        }

        void SetUpEditorFixtureImpl() override
        {
            InitProperties();
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
            g_globalEntityName = my_entity:GetName()
            )LUA");

        EXPECT_NE(g_globalEntityId, AZ::EntityId{});
        EXPECT_STREQ(g_globalEntityName.c_str(), "test");

        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        ASSERT_NE(entity, nullptr);
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
            g_globalComponentId = EntityUtilityBus.Broadcast.GetOrAddComponentByTypeName(ent_id, "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent")
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
}

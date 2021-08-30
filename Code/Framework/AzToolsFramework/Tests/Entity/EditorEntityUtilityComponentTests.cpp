/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class EditorEntityUtilityComponentTests
        : public ToolsApplicationFixture
    {
        
    };

    AZ::EntityId g_globalEntityId = AZ::EntityId{};
    AZStd::string g_globalEntityName = "";

    TEST_F(EditorEntityUtilityComponentTests, Create)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        behaviorContext->Property("g_globalEntityId", BehaviorValueProperty(&g_globalEntityId));
        behaviorContext->Property("g_globalEntityName", BehaviorValueProperty(&g_globalEntityName));

        g_globalEntityId = AZ::EntityId{};
        g_globalEntityName = "";

        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            g_globalEntityId = EditorEntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            my_entity = Entity(g_globalEntityId)
            g_globalEntityName = my_entity:GetName()
            )LUA");

        EXPECT_NE(g_globalEntityId, AZ::EntityId{});
        EXPECT_STREQ(g_globalEntityName.c_str(), "test");

        AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(g_globalEntityId);

        ASSERT_NE(entity, nullptr);
    }

}

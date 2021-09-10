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
    using PrefabScriptingTest = PrefabTestFixture;

    TemplateId g_globalTemplateId = {};

    TEST_F(PrefabScriptingTest, PrefabScripting_CreatePrefab)
    {
        AZ::ScriptContext sc;
        auto behaviorContext = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->GetBehaviorContext();

        behaviorContext->Property("g_globalTemplateId", BehaviorValueProperty(&g_globalTemplateId));

        g_globalTemplateId = TemplateId{};
        
        sc.BindTo(behaviorContext);
        sc.Execute(R"LUA(
            my_id = EditorEntityUtilityBus.Broadcast.CreateEditorReadyEntity("test")
            entities = vector_EntityId(my_id)
            g_globalTemplateId = PrefabSystemScriptingBus.Broadcast.CreatePrefab(entities, "test.prefab", true)
            )LUA");

        EXPECT_NE(g_globalTemplateId, TemplateId{});

        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);

        TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(g_globalTemplateId);

        EXPECT_TRUE(templateRef);
    }
    
}

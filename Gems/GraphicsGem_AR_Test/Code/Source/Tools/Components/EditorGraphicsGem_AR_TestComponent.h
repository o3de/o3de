/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Components/GraphicsGem_AR_TestComponent.h>

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>

namespace GraphicsGem_AR_Test
{
    inline constexpr AZ::TypeId EditorComponentTypeId { "{2E715EC1-EA37-4940-A932-2CD8C4324A4E}" };

    class EditorGraphicsGem_AR_TestComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<GraphicsGem_AR_TestComponentController, GraphicsGem_AR_TestComponent, GraphicsGem_AR_TestComponentConfig>
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TickBus::Handler
        , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<GraphicsGem_AR_TestComponentController, GraphicsGem_AR_TestComponent, GraphicsGem_AR_TestComponentConfig>;
        AZ_EDITOR_COMPONENT(EditorGraphicsGem_AR_TestComponent, EditorComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        EditorGraphicsGem_AR_TestComponent();
        EditorGraphicsGem_AR_TestComponent(const GraphicsGem_AR_TestComponentConfig& config);

        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;

    private:

        // AZ::TickBus overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;


    };
}

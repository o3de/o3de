/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Components/${Name}Component.h>

#include <${Name}/${Name}TypeIds.h>

namespace ${Name}
{
    inline constexpr AZ::TypeId EditorComponentTypeId { "{${Random_Uuid}}" };

    class Editor${Name}Component final
        : public AZ::Render::EditorRenderComponentAdapter<${Name}ComponentController, ${Name}Component, ${Name}ComponentConfig>
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TickBus::Handler
        , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    {
    public:
        using BaseClass = AZ::Render::EditorRenderComponentAdapter <${Name}ComponentController, ${Name}Component, ${Name}ComponentConfig>;
        AZ_EDITOR_COMPONENT(Editor${Name}Component, EditorComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        Editor${Name}Component();
        Editor${Name}Component(const ${Name}ComponentConfig& config);

        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;

    private:

        // AZ::TickBus overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;


    };
}

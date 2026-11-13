/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Component/TickBus.h>
#include <SceneColorView/Configuration.h>

namespace SceneColorView
{
    class EditorSceneColorViewComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , private AZ::TickBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorSceneColorViewComponent, "{5A655248-3869-46C1-8A6F-48F02F8557F0}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* entity) override;
    private:
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override { return AZ::TICK_PRE_RENDER; }
        Configuration m_configuration;
        bool m_warned = false;
    };
}

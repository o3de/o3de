/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace Terrain
{
    /// System component for Terrain editor
    class EditorTerrainSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorTerrainSystemComponent, "{5E9f2200-9099-4325-BABD-6A533A1ABEA8}");
        static void Reflect(AZ::ReflectContext* context);

        EditorTerrainSystemComponent() = default;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("TerrainEditorService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TerrainService"));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // ActionManagerRegistrationNotificationBus overrides ...
        // Puts the Terrain entity preset into the editor's "Create Preset" menu. That preset is
        // hand-written rather than data because it touches the level entity and builds a
        // parent/child pair that refer to each other - see TerrainPreset.
        void OnMenuRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;
    };
} // namespace Terrain

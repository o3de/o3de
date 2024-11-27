/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <CoreLights/AreaLightComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorAreaLightComponent final
            : public EditorRenderComponentAdapter<AreaLightComponentController, AreaLightComponent, AreaLightComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<AreaLightComponentController, AreaLightComponent, AreaLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorAreaLightComponent, EditorAreaLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorAreaLightComponent() = default;
            EditorAreaLightComponent(const AreaLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() const override;
            AZ::Aabb GetLocalBounds() const override;

        private:
            // AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides (inherited from EditorRenderComponentAdapter)
            void OnEntityVisibilityChanged(bool visibility) override;

            // EditorRenderComponentAdapter overrides...
            bool ShouldActivateController() const override;

            bool HandleLightTypeChange();

            u32 OnConfigurationChanged() override;

            AreaLightComponentConfig::LightType m_lightType {AreaLightComponentConfig::LightType::Unknown}; // Used to detect when the configuration's light type changes.
        };

    } // namespace Render
} // namespace AZ

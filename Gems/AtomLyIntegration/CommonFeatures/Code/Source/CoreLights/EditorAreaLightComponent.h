/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once
#include <AzCore/PlatformDef.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <CoreLights/AreaLightComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorAreaLightComponent final
            : public EditorRenderComponentAdapter<AreaLightComponentController, AreaLightComponent, AreaLightComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<AreaLightComponentController, AreaLightComponent, AreaLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorAreaLightComponent, EditorAreaLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorAreaLightComponent() = default;
            EditorAreaLightComponent(const AreaLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

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

            AreaLightComponentConfig::LightType m_lightType; // Used to detect when the configuration's light type changes.
        };

    } // namespace Render
} // namespace AZ

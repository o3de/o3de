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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <CoreLights/SpotLightComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorSpotLightComponent final
            : public EditorRenderComponentAdapter<SpotLightComponentController, SpotLightComponent, SpotLightComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<SpotLightComponentController, SpotLightComponent, SpotLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorSpotLightComponent, EditorSpotLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorSpotLightComponent() = default;
            EditorSpotLightComponent(const SpotLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            // AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // EditorComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/DirectionalLightComponent.h>
#include <Atom/Feature/CoreLights/DirectionalLightShadowNotificationBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightComponentConfig.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AZ
{
    namespace Render
    {
        class EditorDirectionalLightComponent final
            : public EditorRenderComponentAdapter<DirectionalLightComponentController, DirectionalLightComponent, DirectionalLightComponentConfig>
            , public ShadowingDirectionalLightNotificationsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<DirectionalLightComponentController, DirectionalLightComponent, DirectionalLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDirectionalLightComponent, EditorDirectionalLightComponentTypeId, BaseClass);

            static void Reflect(ReflectContext* context);

            EditorDirectionalLightComponent() = default;
            EditorDirectionalLightComponent(const DirectionalLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

            // ShadowingDirectionalLightNotificationsBus overrides
            void OnShadowingDirectionalLightChanged(const DirectionalLightFeatureProcessorInterface::LightHandle& handle) override;

            //! AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;
        };

    } // namespace Render
} // namespace AZ

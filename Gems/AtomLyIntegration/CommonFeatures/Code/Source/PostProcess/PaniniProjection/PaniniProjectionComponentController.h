/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionConstants.h>
#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/PaniniProjection/PaniniProjectionBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PaniniProjection/PaniniProjectionComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace AZ
{
    namespace Render
    {
        class PaniniProjectionComponentController final : public PaniniProjectionRequestBus::Handler
        {
        public:
            friend class EditorPaniniProjectionComponent;

            AZ_TYPE_INFO(AZ::Render::PaniniProjectionComponentController, "{15B93DBF-D7E4-4F39-94BB-5F97606CF858}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            PaniniProjectionComponentController() = default;
            PaniniProjectionComponentController(const PaniniProjectionComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PaniniProjectionComponentConfig& config);
            const PaniniProjectionComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(PaniniProjectionComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            PaniniProjectionSettingsInterface* m_settingsInterface = nullptr;
            PaniniProjectionComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ

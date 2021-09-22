/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ColorGrading/HDRColorGradingComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ColorGrading/HDRColorGradingBus.h>

#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class HDRColorGradingComponentController final
            : public HDRColorGradingRequestBus::Handler
        {
        public:
            friend class EditorHDRColorGradingComponent;

            AZ_TYPE_INFO(AZ::Render::HDRColorGradingComponentController, "{CA1D635C-64E9-42C7-A8E0-36C6B825B15D}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            HDRColorGradingComponentController() = default;
            HDRColorGradingComponentController(const HDRColorGradingComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const HDRColorGradingComponentConfig& config);
            const HDRColorGradingComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(HDRColorGradingComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            HDRColorGradingSettingsInterface* m_settingsInterface = nullptr;
            HDRColorGradingComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ

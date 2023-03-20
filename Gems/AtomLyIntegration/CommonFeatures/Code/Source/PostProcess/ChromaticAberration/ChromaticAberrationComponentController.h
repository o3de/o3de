/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/ChromaticAberration/ChromaticAberrationBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ChromaticAberration/ChromaticAberrationComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace AZ
{
    namespace Render
    {
        class ChromaticAberrationComponentController final : public ChromaticAberrationRequestBus::Handler
        {
        public:
            friend class EditorChromaticAberrationComponent;

            AZ_TYPE_INFO(AZ::Render::ChromaticAberrationComponentController, "{776770B4-03BA-491D-BE5B-CBF3948BF078}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            ChromaticAberrationComponentController() = default;
            ChromaticAberrationComponentController(const ChromaticAberrationComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ChromaticAberrationComponentConfig& config);
            const ChromaticAberrationComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(ChromaticAberrationComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            ChromaticAberrationSettingsInterface* m_settingsInterface = nullptr;
            ChromaticAberrationComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ

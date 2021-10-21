/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/EditorModeFeedback/EditorModeFeedbackComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/EditorModeFeedback/EditorModeFeedbackBus.h>

#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class EditorModeFeedbackComponentController final
            : public EditorModeFeedbackRequestBus::Handler
        {
        public:
            friend class EditorEditorModeFeedbackComponent;

            AZ_TYPE_INFO(AZ::Render::EditorModeFeedbackComponentController, "{8523D0BC-2193-4E62-9254-644BD0868D8E}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            EditorModeFeedbackComponentController() = default;
            EditorModeFeedbackComponentController(const EditorModeFeedbackComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const EditorModeFeedbackComponentConfig& config);
            const EditorModeFeedbackComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(EditorModeFeedbackComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            EditorModeFeedbackSettingsInterface* m_settingsInterface = nullptr;
            EditorModeFeedbackComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ

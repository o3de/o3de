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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/DepthOfField/DepthOfFieldBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DepthOfField/DepthOfFieldComponentConfig.h>

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DepthOfFieldComponentController final
            : public DepthOfFieldRequestBus::Handler
        {
        public:
            friend class EditorDepthOfFieldComponent;

            AZ_TYPE_INFO(AZ::Render::DepthOfFieldComponentController, "{D0E1675C-7E6F-472D-B037-E0D7ED5AFBE8}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DepthOfFieldComponentController() = default;
            DepthOfFieldComponentController(const DepthOfFieldComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DepthOfFieldComponentConfig& config);
            const DepthOfFieldComponentConfig& GetConfiguration() const;

            // Updates members that are inferred from the values of other parameters
            void UpdateInferredParams();

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(DepthOfFieldComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            DepthOfFieldSettingsInterface* m_depthOfFieldSettingsInterface = nullptr;
            DepthOfFieldComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}

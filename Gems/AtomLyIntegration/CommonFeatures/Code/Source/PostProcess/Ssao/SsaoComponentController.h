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

#include <AtomLyIntegration/CommonFeatures/PostProcess/Ssao/SsaoBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/Ssao/SsaoComponentConfiguration.h>

#include <Atom/Feature/PostProcess/Ssao/SsaoSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class SsaoComponentController final
            : public SsaoRequestBus::Handler
        {
        public:
            friend class EditorSsaoComponent;

            AZ_TYPE_INFO(AZ::Render::SsaoComponentController, "{B53B6F29-C803-46AD-83E1-526457BDFBAE}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            SsaoComponentController() = default;
            SsaoComponentController(const SsaoComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const SsaoComponentConfig& config);
            const SsaoComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(SsaoComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            SsaoSettingsInterface* m_ssaoSettingsInterface = nullptr;
            SsaoComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}

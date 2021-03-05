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
#include <AtomLyIntegration/CommonFeatures/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxWeightRequestBus.h>

namespace AZ
{
    namespace Render
    {
        class RadiusWeightModifierComponentController final
            : public PostFxWeightRequestBus::Handler
        {
        public:
            friend class EditorRadiusWeightModifierComponent;

            AZ_TYPE_INFO(AZ::Render::RadiusWeightModifierComponentController, "{29565EC9-8DE1-46A5-B20C-328AA6ED23C6}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            RadiusWeightModifierComponentController() = default;
            RadiusWeightModifierComponentController(const RadiusWeightModifierComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const RadiusWeightModifierComponentConfig& config);
            const RadiusWeightModifierComponentConfig& GetConfiguration() const { return m_configuration; }
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const override;
        private:
            AZ_DISABLE_COPY(RadiusWeightModifierComponentController);

            RadiusWeightModifierComponentConfig m_configuration;

            EntityId m_entityId;
        };
    }
}

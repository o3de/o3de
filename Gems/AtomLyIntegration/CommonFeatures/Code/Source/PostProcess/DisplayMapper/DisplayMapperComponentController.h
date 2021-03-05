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

#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>

#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DisplayMapperComponentController final
        {
        public:
            friend class EditorDisplayMapperComponent;

            AZ_TYPE_INFO(AZ::Render::DisplayMapperComponentController, "{85E5AE10-68AD-462D-B389-B8748BB1A19C}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DisplayMapperComponentController() = default;
            DisplayMapperComponentController(const DisplayMapperComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DisplayMapperComponentConfig& config);
            const DisplayMapperComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(DisplayMapperComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            DisplayMapperComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}

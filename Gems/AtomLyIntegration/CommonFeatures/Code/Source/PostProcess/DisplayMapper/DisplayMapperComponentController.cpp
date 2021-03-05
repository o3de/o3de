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

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/DisplayMapper/DisplayMapperComponentController.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

namespace AZ
{
    namespace Render
    {
        void DisplayMapperComponentController::Reflect(ReflectContext* context)
        {
            DisplayMapperComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DisplayMapperComponentController::m_configuration);
            }
        }

        void DisplayMapperComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ToneMapperService", 0xb8f814e8));
        }

        void DisplayMapperComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ToneMapperService", 0xb8f814e8));
        }

        void DisplayMapperComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        DisplayMapperComponentController::DisplayMapperComponentController(const DisplayMapperComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DisplayMapperComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
        }

        void DisplayMapperComponentController::Deactivate()
        {
            m_postProcessInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void DisplayMapperComponentController::SetConfiguration(const DisplayMapperComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const DisplayMapperComponentConfig& DisplayMapperComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DisplayMapperComponentController::OnConfigChanged()
        {
            // Register the configuration with the  AcesDisplayMapperFeatureProcessor for this scene.
            const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
            DisplayMapperFeatureProcessorInterface* fp = scene->GetFeatureProcessor<DisplayMapperFeatureProcessorInterface>();
            DisplayMapperConfigurationDescriptor desc;
            desc.m_operationType = m_configuration.m_displayMapperOperation;
            desc.m_ldrGradingLutEnabled = m_configuration.m_ldrColorGradingLutEnabled;
            desc.m_ldrColorGradingLut = m_configuration.m_ldrColorGradingLut;
            fp->RegisterDisplayMapperConfiguration(desc);
        }

    } // namespace Render
} // namespace AZ

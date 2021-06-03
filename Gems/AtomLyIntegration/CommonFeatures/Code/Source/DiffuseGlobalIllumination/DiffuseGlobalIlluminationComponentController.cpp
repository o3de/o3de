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

//#include <Atom/RPI.Public/Scene.h>

#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentController.h>
//#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        void DiffuseGlobalIlluminationComponentController::Reflect(ReflectContext* context)
        {
            DiffuseGlobalIlluminationComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DiffuseGlobalIlluminationComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DiffuseGlobalIlluminationComponentController::m_configuration);
            }
        }

        void DiffuseGlobalIlluminationComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("DiffuseGlobalIlluminationService", 0x11b9cbe1));
        }

        void DiffuseGlobalIlluminationComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("DiffuseGlobalIlluminationService", 0x11b9cbe1));
        }

        void DiffuseGlobalIlluminationComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        DiffuseGlobalIlluminationComponentController::DiffuseGlobalIlluminationComponentController(const DiffuseGlobalIlluminationComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DiffuseGlobalIlluminationComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
        }

        void DiffuseGlobalIlluminationComponentController::Deactivate()
        {
            //m_postProcessInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void DiffuseGlobalIlluminationComponentController::SetConfiguration(const DiffuseGlobalIlluminationComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const DiffuseGlobalIlluminationComponentConfig& DiffuseGlobalIlluminationComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DiffuseGlobalIlluminationComponentController::OnConfigChanged()
        {
            // Register the configuration with the  AcesDisplayMapperFeatureProcessor for this scene.
            //const AZ::RPI::Scene* scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get();
            //DisplayMapperFeatureProcessorInterface* fp = scene->GetFeatureProcessor<DisplayMapperFeatureProcessorInterface>();
            //DisplayMapperConfigurationDescriptor desc;
            //desc.m_operationType = m_configuration.m_displayMapperOperation;
            //desc.m_ldrGradingLutEnabled = m_configuration.m_ldrColorGradingLutEnabled;
            //desc.m_ldrColorGradingLut = m_configuration.m_ldrColorGradingLut;
            //desc.m_acesParameterOverrides = m_configuration.m_acesParameterOverrides;
            //fp->RegisterDisplayMapperConfiguration(desc);
        }

    } // namespace Render
} // namespace AZ

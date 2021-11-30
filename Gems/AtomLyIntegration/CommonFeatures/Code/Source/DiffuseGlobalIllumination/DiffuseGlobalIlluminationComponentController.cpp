/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentController.h>

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
            m_featureProcessor = AZ::RPI::Scene::GetFeatureProcessorForEntity<DiffuseGlobalIlluminationFeatureProcessorInterface>(entityId);
            OnConfigChanged();
        }

        void DiffuseGlobalIlluminationComponentController::Deactivate()
        {
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
            m_featureProcessor->SetQualityLevel(m_configuration.m_qualityLevel);
        }

    } // namespace Render
} // namespace AZ

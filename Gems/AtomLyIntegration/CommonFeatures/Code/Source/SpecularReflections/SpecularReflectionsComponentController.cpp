/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <SpecularReflections/SpecularReflectionsComponentController.h>

namespace AZ
{
    namespace Render
    {
        void SpecularReflectionsComponentController::Reflect(ReflectContext* context)
        {
            SpecularReflectionsComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &SpecularReflectionsComponentController::m_configuration);
            }
        }

        void SpecularReflectionsComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ReflectionsService"));
        }

        void SpecularReflectionsComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ReflectionsService"));
        }

        void SpecularReflectionsComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        SpecularReflectionsComponentController::SpecularReflectionsComponentController(const SpecularReflectionsComponentConfig& config)
            : m_configuration(config)
        {
        }

        void SpecularReflectionsComponentController::Activate(EntityId entityId)
        {
            AZ::ApplicationTypeQuery appType;
            ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (appType.IsHeadless())
            {
                return;
            }

            m_featureProcessor = AZ::RPI::Scene::GetFeatureProcessorForEntity<SpecularReflectionsFeatureProcessorInterface>(entityId);
            OnConfigChanged();
        }

        void SpecularReflectionsComponentController::Deactivate()
        {
        }

        void SpecularReflectionsComponentController::SetConfiguration(const SpecularReflectionsComponentConfig& config)
        {
            m_configuration = config;

            OnConfigChanged();
        }

        const SpecularReflectionsComponentConfig& SpecularReflectionsComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void SpecularReflectionsComponentController::OnConfigChanged()
        {
            m_featureProcessor->SetSSROptions(m_configuration.m_ssr.m_options);
        }

    } // namespace Render
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
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

        void DiffuseGlobalIlluminationComponentController::Activate([[maybe_unused]] EntityId entityId)
        {
            // DiffuseGlobalIllumination settings are global settings that should be applied
            // to the scene associated with the main game entity context
            AzFramework::EntityContextId entityContextId;
            AzFramework::GameEntityContextRequestBus::BroadcastResult(
                entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

            // GetFeaturePcoressorForEntityId won't work here because the DiffuseGlobalIlluminationComponent is part of the level entity,
            // and the level entity is part of the EditorEntityContext, not the GameEntityContext
            m_featureProcessor =
                AZ::RPI::Scene::GetFeatureProcessorForEntityContextId<DiffuseGlobalIlluminationFeatureProcessorInterface>(entityContextId);

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

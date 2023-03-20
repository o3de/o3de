/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentController.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void RadiusWeightModifierComponentController::Reflect(AZ::ReflectContext* context)
        {
            RadiusWeightModifierComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RadiusWeightModifierComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &RadiusWeightModifierComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PostFxWeightRequestBus>("PostFxWeightRequestBus");
            }
        }

        void RadiusWeightModifierComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void RadiusWeightModifierComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void RadiusWeightModifierComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        RadiusWeightModifierComponentController::RadiusWeightModifierComponentController(const RadiusWeightModifierComponentConfig& config)
            : m_configuration(config)
        {

        }

        void RadiusWeightModifierComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostFxWeightRequestBus::Handler::BusConnect(m_entityId);
        }

        void RadiusWeightModifierComponentController::Deactivate()
        {
            PostFxWeightRequestBus::Handler::BusDisconnect(m_entityId);

            m_entityId.SetInvalid();
        }

        void RadiusWeightModifierComponentController::SetConfiguration(const RadiusWeightModifierComponentConfig& config)
        {
            m_configuration = config;
        }

        float RadiusWeightModifierComponentController::GetWeightAtPosition(const AZ::Vector3& influencerPosition) const
        {
            AZ::Vector3 postfxCenterPosition = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(postfxCenterPosition, m_entityId, &AZ::TransformInterface::GetWorldTranslation);
            float magnitude = postfxCenterPosition.GetDistance(influencerPosition);
            return AZ::GetMax(0.0f, 1 - magnitude / m_configuration.m_radius);
        }
    }
}

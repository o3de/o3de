/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <PostProcess/GradientWeightModifier/GradientWeightModifierController.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void GradientWeightModifierComponentController::Reflect(AZ::ReflectContext* context)
        {
            GradientWeightModifierComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GradientWeightModifierComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &GradientWeightModifierComponentController::m_configuration);
            }
        }

        void GradientWeightModifierComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void GradientWeightModifierComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void GradientWeightModifierComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        GradientWeightModifierComponentController::GradientWeightModifierComponentController(const GradientWeightModifierComponentConfig& config)
            : m_configuration(config)
        {

        }

        void GradientWeightModifierComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_configuration.m_gradientSampler.m_ownerEntityId = entityId;

            PostFxWeightRequestBus::Handler::BusConnect(m_entityId);
        }

        void GradientWeightModifierComponentController::Deactivate()
        {
            PostFxWeightRequestBus::Handler::BusDisconnect(m_entityId);

            m_entityId.SetInvalid();
        }

        void GradientWeightModifierComponentController::SetConfiguration(const GradientWeightModifierComponentConfig& config)
        {
            m_configuration = config;
        }

        float GradientWeightModifierComponentController::GetWeightAtPosition(const AZ::Vector3& influencerPosition) const
        {
            return m_configuration.m_gradientSampler.GetValue(influencerPosition);
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentController.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace AZ
{
    namespace Render
    {
        void ShapeWeightModifierComponentController::Reflect(AZ::ReflectContext* context)
        {
            ShapeWeightModifierComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShapeWeightModifierComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &ShapeWeightModifierComponentController::m_configuration);
            }
        }

        void ShapeWeightModifierComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void ShapeWeightModifierComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PostFXWeightModifierService"));
        }

        void ShapeWeightModifierComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
            required.push_back(AZ_CRC_CE("ShapeService"));
        }

        ShapeWeightModifierComponentController::ShapeWeightModifierComponentController(const ShapeWeightModifierComponentConfig& config)
            : m_configuration(config)
        {

        }

        void ShapeWeightModifierComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostFxWeightRequestBus::Handler::BusConnect(m_entityId);
        }

        void ShapeWeightModifierComponentController::Deactivate()
        {
            PostFxWeightRequestBus::Handler::BusDisconnect(m_entityId);

            m_entityId.SetInvalid();
        }

        void ShapeWeightModifierComponentController::SetConfiguration(const ShapeWeightModifierComponentConfig& config)
        {
            m_configuration = config;
        }

        // Implementation taken from GradientSignal Gem
        float ShapeWeightModifierComponentController::GetWeightAtPosition(const AZ::Vector3& influencerPosition) const
        {
            float distance = 0.0f;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, m_entityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, influencerPosition);

            // In the special case of 0 falloff, make sure that all points inside the shape (0 distance) return 
            // 1.0, and all points outside the shape return 0.
            if (m_configuration.m_falloffDistance == 0.0f)
            {
                return (distance > 0.0f) ? 0.0f : 1.0f;
            }

            // Since this is outer falloff, distance should give us values from 1.0 at the minimum distance
            // to 0.0 at the maximum distance.
            return GetRatio(m_configuration.m_falloffDistance, 0.0f, distance);
        }

        // Implementation taken from GradientSignal Gem
        float ShapeWeightModifierComponentController::GetRatio(float maxRange, float minRange, float distance) const
        {
            // If our min/max range is equal, GetRatio() would end up dividing by infinity, so in this case
            // make sure that everything below or equal to the min/max value is 0.0, and everything above it is 1.0.
            if (maxRange == minRange)
            {
                return (distance <= minRange) ? 0.0f : 1.0f;
            }

            return AZ::GetClamp((distance - maxRange) / (minRange - maxRange), 0.0f, 1.0f);
        }
    }
}

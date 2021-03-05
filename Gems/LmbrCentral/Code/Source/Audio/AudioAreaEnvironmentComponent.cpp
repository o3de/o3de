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

#include "LmbrCentral_precompiled.h"
#include "AudioAreaEnvironmentComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/WorldEventhandler.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <ISystem.h>

namespace LmbrCentral
{
    //=========================================================================
    void AudioAreaEnvironmentComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AudioAreaEnvironmentComponent, AZ::Component>()
                ->Version(1)
                ->Field("Broad-phase Trigger Area entity", &AudioAreaEnvironmentComponent::m_broadPhaseTriggerArea)
                ->Field("Environment name", &AudioAreaEnvironmentComponent::m_environmentName)
                ->Field("Environment fade distance", &AudioAreaEnvironmentComponent::m_environmentFadeDistance)
                ;
        }
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::Activate()
    {
        m_environmentId = INVALID_AUDIO_ENVIRONMENT_ID;
        if (!m_environmentName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_environmentId, &Audio::AudioSystemRequestBus::Events::GetAudioEnvironmentID, m_environmentName.c_str());
        }

        if (m_broadPhaseTriggerArea.IsValid())
        {
            Physics::TriggerNotificationBus::Handler::BusConnect(m_broadPhaseTriggerArea);
        }
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::Deactivate()
    {
        Physics::TriggerNotificationBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (m_environmentId == INVALID_AUDIO_ENVIRONMENT_ID)
        {
            AZ_WarningOnce("AudioAreaEnvironmentComponent", m_environmentId != INVALID_AUDIO_ENVIRONMENT_ID,
                "AudioAreaEnvironmentComponent - Invalid Environment being used!");
            return;
        }

        const AZ::EntityId* busEntityId = AZ::TransformNotificationBus::GetCurrentBusId();
        if (!busEntityId)
        {
            AZ_ErrorOnce("AudioAreaEnvironmentComponent", busEntityId != nullptr,
                "AudioAreaEnvironmentComponent - Bus Id is null!");
            return;
        }

        AZ::Vector3 entityPos = world.GetTranslation();
        float distanceFromShape = 0.f;
        ShapeComponentRequestsBus::EventResult(distanceFromShape, GetEntityId(), &ShapeComponentRequestsBus::Events::DistanceFromPoint, entityPos);

        // Calculate a fade value to pass as the environment amount for the entity.
        // Linear fade is fine, the audio middleware can be authored to translate this into custom curves.
        float fadeValue = AZ::GetClamp(distanceFromShape, 0.f, m_environmentFadeDistance);
        fadeValue = (1.f - (fadeValue / m_environmentFadeDistance));

        AudioProxyComponentRequestBus::Event(*busEntityId, &AudioProxyComponentRequestBus::Events::SetEnvironmentAmount, m_environmentId, fadeValue);
    }


    //=========================================================================
    void AudioAreaEnvironmentComponent::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        AZ::EntityId enteringEntityId = triggerEvent.m_otherBody->GetEntityId();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(enteringEntityId);
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        AZ::EntityId exitingEntityId = triggerEvent.m_otherBody->GetEntityId();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(exitingEntityId);

        if (m_environmentId != INVALID_AUDIO_ENVIRONMENT_ID)
        {
            // When entities fully exit the broad-phase trigger area, set the environment amount to zero to ensure no effects linger on the entity.
            AudioProxyComponentRequestBus::Event(exitingEntityId, &AudioProxyComponentRequestBus::Events::SetEnvironmentAmount, m_environmentId, 0.f);
        }
    }

} // namespace LmbrCentral

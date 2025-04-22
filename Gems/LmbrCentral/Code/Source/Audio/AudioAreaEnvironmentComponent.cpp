/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioAreaEnvironmentComponent.h"

#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <ISystem.h>

namespace LmbrCentral
{
    AudioAreaEnvironmentComponent::AudioAreaEnvironmentComponent()
        : m_onTriggerEnterHandler([this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
            const  AzPhysics::TriggerEvent& triggerEvent)
            {
                OnTriggerEnter(triggerEvent);
            })
        , m_onTriggerExitHandler([this]([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
            const  AzPhysics::TriggerEvent& triggerEvent)
            {
                OnTriggerExit(triggerEvent);
            })
    {

    }

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
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
            {
                m_environmentId = audioSystem->GetAudioEnvironmentID(m_environmentName.c_str());
            }
        }

        if (m_broadPhaseTriggerArea.IsValid())
        {
            // During entity activation the simulated bodies are not created yet.
            // Connect to RigidBodyNotificationBus to listen when they get enabled to register the trigger handlers.
            Physics::RigidBodyNotificationBus::Handler::BusConnect(m_broadPhaseTriggerArea);
        }
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::Deactivate()
    {
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();
        m_onTriggerEnterHandler.Disconnect();
        m_onTriggerExitHandler.Disconnect();
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::OnPhysicsEnabled(const AZ::EntityId& entityId)
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> foundBody =
                physicsSystem->FindAttachedBodyHandleFromEntityId(entityId);
            if (foundBody.first != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(foundBody.first, foundBody.second, m_onTriggerEnterHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerExitHandler(foundBody.first, foundBody.second, m_onTriggerExitHandler);
            }
        }
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::OnPhysicsDisabled([[maybe_unused]] const AZ::EntityId& entityId)
    {
        m_onTriggerEnterHandler.Disconnect();
        m_onTriggerExitHandler.Disconnect();
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
    void AudioAreaEnvironmentComponent::OnTriggerEnter(const AzPhysics::TriggerEvent& triggerEvent)
    {
        AZ::EntityId enteringEntityId = triggerEvent.m_otherBody->GetEntityId();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(enteringEntityId);
    }

    //=========================================================================
    void AudioAreaEnvironmentComponent::OnTriggerExit(const AzPhysics::TriggerEvent& triggerEvent)
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

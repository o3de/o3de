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
#include "AudioMultiPositionComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

namespace LmbrCentral
{
    //=========================================================================
    void AudioMultiPositionComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AudioMultiPositionComponent, AZ::Component>()
                ->Version(0)
                ->Field("Entity Refs", &AudioMultiPositionComponent::m_entityRefs)
                ->Field("Behavior Type", &AudioMultiPositionComponent::m_behaviorType)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Enum<static_cast<AZ::u32>(Audio::MultiPositionBehaviorType::Separate)>("MultiPositionBehaviorType_Separate")
                ->Enum<static_cast<AZ::u32>(Audio::MultiPositionBehaviorType::Blended)>("MultiPositionBehaviorType_Blended")
                ;

            behaviorContext->EBus<AudioMultiPositionComponentRequestBus>("Multi-Position Audio Requests", "AudioMultiPositionComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Audio")
                ->Event("Add Entity", &AudioMultiPositionComponentRequestBus::Events::AddEntity, "AddEntity")
                ->Event("RemoveEntity", &AudioMultiPositionComponentRequestBus::Events::RemoveEntity)
                ->Event("SetBehaviorType", &AudioMultiPositionComponentRequestBus::Events::SetBehaviorType)
                ;
        }
    }

    //=========================================================================
    AudioMultiPositionComponent::AudioMultiPositionComponent(const AZStd::vector<AZ::EntityId>& entities, Audio::MultiPositionBehaviorType type)
        : m_behaviorType(type)
    {
        // Remove duplicates when building the game entity.
        AZStd::unordered_set<AZ::EntityId> setOfEntities(entities.begin(), entities.end());
        m_entityRefs.assign(setOfEntities.begin(), setOfEntities.end());

        m_entityPositions.reserve(m_entityRefs.size());
    }

    //=========================================================================
    void AudioMultiPositionComponent::Activate()
    {
        for (const auto& entityId : m_entityRefs)
        {
            AZ::EntityBus::MultiHandler::BusConnect(entityId);
        }

        AudioMultiPositionComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioMultiPositionComponent::Deactivate()
    {
        AudioMultiPositionComponentRequestBus::Handler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();
    }

    //=========================================================================
    void AudioMultiPositionComponent::SetBehaviorType(Audio::MultiPositionBehaviorType type)
    {
        if (m_behaviorType != type)
        {
            m_behaviorType = type;

            // Re-send if all entities are activated...
            if (m_entityPositions.size() == m_entityRefs.size())
            {
                SendMultiplePositions();
            }
        }
    }

    //=========================================================================
    void AudioMultiPositionComponent::AddEntity(const AZ::EntityId& entityId)
    {
        auto iter = AZStd::find(m_entityRefs.begin(), m_entityRefs.end(), entityId);
        if (iter == m_entityRefs.end())
        {
            m_entityRefs.push_back(entityId);
            AZ::EntityBus::MultiHandler::BusConnect(entityId);
        }
    }

    //=========================================================================
    void AudioMultiPositionComponent::RemoveEntity(const AZ::EntityId& entityId)
    {
        auto iter = AZStd::find(m_entityRefs.begin(), m_entityRefs.end(), entityId);
        if (iter != m_entityRefs.end())
        {
            m_entityRefs.erase(iter);
            AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
        }
    }

    //=========================================================================
    void AudioMultiPositionComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ_Assert(m_entityPositions.size() < m_entityRefs.size(), "Multi-Position Audio: Seen more entities activated than entities being tracked.\n");
        AZ::Vector3 position;
        AZ::TransformBus::EventResult(position, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        m_entityPositions.push_back(AZStd::make_pair(entityId, position));

        if (m_entityPositions.size() == m_entityRefs.size())
        {
            SendMultiplePositions();
        }
    }

    //=========================================================================
    void AudioMultiPositionComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        auto iter = AZStd::find_if(m_entityPositions.begin(), m_entityPositions.end(),
            [&entityId](const auto& entityPositionPair) -> bool
            {
                return entityPositionPair.first == entityId;
            }
        );

        if (iter != m_entityPositions.end())
        {
            m_entityPositions.erase(iter);
        }
    }

    //=========================================================================
    void AudioMultiPositionComponent::SendMultiplePositions()
    {
        // Sends the vector of positions out to be processed.
        Audio::MultiPositionParams params;
        params.m_type = m_behaviorType;

        for (const auto& entityPosPair : m_entityPositions)
        {
            params.m_positions.emplace_back(entityPosPair.second);
        }

        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetMultiplePositions, params);
    }

} // namespace LmbrCentral

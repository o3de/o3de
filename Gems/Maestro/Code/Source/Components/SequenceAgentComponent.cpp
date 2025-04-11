/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SequenceAgentComponent.h"

#include <AzFramework/API/ApplicationAPI.h>

namespace Maestro
{
    namespace ClassConverters
    {
        static bool UpgradeSequenceAgentComponent(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
    } // namespace ClassConverters

    /*static*/ void SequenceAgentComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SequenceAgentComponent, Component>()
                ->Field("SequenceComponentEntityIds", &SequenceAgentComponent::m_sequenceEntityIds)
                ->Version(2, &ClassConverters::UpgradeSequenceAgentComponent)
                ;
        }
    }

    void SequenceAgentComponent::Init()
    {
        m_addressToBehaviorVirtualPropertiesMap.clear();
    }

    void SequenceAgentComponent::Activate()
    {
        // cache pointers and animatable addresses for animation
        //
        CacheAllVirtualPropertiesFromBehaviorContext();

        ConnectAllSequences();
    }

    void SequenceAgentComponent::Deactivate()
    {
        // invalidate all cached pointers and addresses for animation   
        m_addressToBehaviorVirtualPropertiesMap.clear();

        DisconnectAllSequences();
    }

    void SequenceAgentComponent::ConnectSequence(const AZ::EntityId& sequenceEntityId)
    {
        if (m_sequenceEntityIds.find(sequenceEntityId) == m_sequenceEntityIds.end())
        {
            m_sequenceEntityIds.insert(sequenceEntityId);
            // connect to EBus between the given SequenceComponent and me
            SequenceAgentEventBusId busId(sequenceEntityId, GetEntityId());
            SequenceAgentComponentRequestBus::MultiHandler::BusConnect(busId);
        }
    }

    void SequenceAgentComponent::DisconnectSequence()
    {
        const SequenceAgentEventBusId *busIdToDisconnect = SequenceAgentComponentRequestBus::GetCurrentBusId();
        if (busIdToDisconnect)
        {
            AZ::EntityId sequenceEntityId = busIdToDisconnect->first;

            // we only process DisconnectSequence events sent over an ID'ed bus - otherwise we don't know which SequenceComponent to disconnect
            [[maybe_unused]] auto findIter = m_sequenceEntityIds.find(sequenceEntityId);
            AZ_Assert(findIter != m_sequenceEntityIds.end(), "A sequence not connected to SequenceAgentComponent on %s is requesting a disconnection", GetEntity()->GetName().c_str());

            m_sequenceEntityIds.erase(sequenceEntityId);

            // Disconnect from the bus between the SequenceComponent and me
            SequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(*busIdToDisconnect);
        }
    }

    void SequenceAgentComponent::ConnectAllSequences()
    {
        // Connect all buses
        for (auto iter = m_sequenceEntityIds.begin(); iter != m_sequenceEntityIds.end(); iter++)
        {
            SequenceAgentEventBusId busIdToConnect(*iter, GetEntityId());
            SequenceAgentComponentRequestBus::MultiHandler::BusConnect(busIdToConnect);
        }
    }

    void SequenceAgentComponent::DisconnectAllSequences()
    {
        // disconnect all buses
        for (auto iter = m_sequenceEntityIds.begin(); iter != m_sequenceEntityIds.end(); iter++)
        {
            SequenceAgentEventBusId busIdToDisconnect(*iter, GetEntityId());
            SequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(busIdToDisconnect);
        }
    }

    void SequenceAgentComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        SequenceAgent::GetAnimatedPropertyValue(returnValue, GetEntityId(), animatableAddress);
    }

    bool SequenceAgentComponent::SetAnimatedPropertyValue(const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        return SequenceAgent::SetAnimatedPropertyValue(GetEntityId(), animatableAddress, value);
    }

    AZ::Uuid SequenceAgentComponent::GetAnimatedAddressTypeId(const AnimatablePropertyAddress& animatableAddress)
    {
        return GetVirtualPropertyTypeId(animatableAddress);
    }

    void SequenceAgentComponent::GetAssetDuration(AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)
    {
        SequenceAgent::GetAssetDuration(returnValue, componentId, assetId);
    }

    void SequenceAgentComponent::GetEntityComponents(AZ::Entity::ComponentArrayType& entityComponents) const
    {
        AZ::Entity* entity = GetEntity();
        AZ_Assert(entity, "Expected valid entity.");
        if (entity)
        {
            const AZ::Entity::ComponentArrayType& enabledComponents = entity->GetComponents();
            for (AZ::Component* component : enabledComponents)
            {
                entityComponents.push_back(component);
            }
        }
    }
 
    namespace ClassConverters
    {
        static bool UpgradeSequenceAgentComponent([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 1)
            {
                // upgrade V1 to V2 - change element named "SequenceEntityComponentPairIds" to "SequenceComponentEntityIds"
                int oldSeqIdNameIdx = classElement.FindElement(AZ::Crc32("SequenceEntityComponentPairIds"));
                if (oldSeqIdNameIdx == -1)
                {
                    AZ_Error("Serialization", false, "Failed to find old SequenceEntityComponentPairIds element.");
                    return false;
                }

                auto seqIdNameElement = classElement.GetSubElement(oldSeqIdNameIdx);
                seqIdNameElement.SetName("SequenceComponentEntityIds");
            }

            return true;
        }
    } // namespace ClassConverters

} // namespace Maestro

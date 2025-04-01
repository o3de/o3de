/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorSequenceAgentComponent.h"
#include "SequenceAgentComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <Maestro/Types/AnimParamType.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>

namespace Maestro
{
    void EditorSequenceAgentComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorSequenceAgentComponent, EditorComponentBase>()
                ->Field("SequenceComponentEntityIds", &EditorSequenceAgentComponent::m_sequenceEntityIds)
                ->Version(3);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorSequenceAgentComponent>(
                    "SequenceAgent", "Maps Director Component Animations to Behavior Properties on this Entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Cinematics")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/SequenceAgent.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/SequenceAgent.png")
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // SequenceAgents are only added by TrackView
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    AZ::TypeId EditorSequenceAgentComponent::GetComponentTypeUuid(const AZ::Component& component) const
    {
        return AzToolsFramework::GetUnderlyingComponentType(component);
    }

    void EditorSequenceAgentComponent::GetEntityComponents(AZ::Entity::ComponentArrayType& entityComponents) const
    {
        AZ::Entity* entity = GetEntity();
        AZ_Assert(entity, "Expected valid entity.");
        if (entity)
        {
            // Add all enabled components
            const AZ::Entity::ComponentArrayType& enabledComponents = entity->GetComponents();
            for (AZ::Component* component : enabledComponents)
            {
                entityComponents.push_back(component);
            }

            // Add all disabled components
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

            for (AZ::Component* component : disabledComponents)
            {
                entityComponents.push_back(component);
            }

            // Add all pending components
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                entityComponents.push_back(component);
            }
        }
    }

    void EditorSequenceAgentComponent::Activate()
    {
        // cache pointers and animatable addresses for animation
        //
        CacheAllVirtualPropertiesFromBehaviorContext();

        ConnectAllSequences();

        EditorComponentBase::Activate();

        // Notify the sequence agent was just connected to the sequence.
        EditorSequenceAgentComponentNotificationBus::Event(
            GetEntityId(),
            &EditorSequenceAgentComponentNotificationBus::Events::OnSequenceAgentConnected
        );
    }

    void EditorSequenceAgentComponent::Deactivate()
    {
        // invalidate all cached pointers and address
        m_addressToBehaviorVirtualPropertiesMap.clear();

        DisconnectAllSequences();

        EditorComponentBase::Deactivate();
    }

    void EditorSequenceAgentComponent::ConnectSequence(const AZ::EntityId& sequenceEntityId)
    {
        // check that we aren't already connected to this SequenceComponent - add it if we aren't
        if (m_sequenceEntityIds.find(sequenceEntityId) == m_sequenceEntityIds.end())
        {
            m_sequenceEntityIds.insert(sequenceEntityId);
            // connect to EBus between the given SequenceComponent and me
            SequenceAgentEventBusId busId(sequenceEntityId, GetEntityId());
            EditorSequenceAgentComponentRequestBus::MultiHandler::BusConnect(busId);
            SequenceAgentComponentRequestBus::MultiHandler::BusConnect(busId);
        }
    }

    void EditorSequenceAgentComponent::DisconnectSequence()
    {   
        const SequenceAgentEventBusId* busIdToDisconnect = SequenceAgentComponentRequestBus::GetCurrentBusId();

        if (!busIdToDisconnect)
        {
            return;
        }

        const auto entity = GetEntity();
        if (!entity)
        {
            AZ_Assert(false, "EditorSequenceAgentComponent::DisconnectSequence() called for inactive entity.");
            return;
        }

        AZ::EntityId sequenceEntityId = busIdToDisconnect->first;

        // we only process DisconnectSequence events sent over an ID'ed bus - otherwise we don't know which SequenceComponent to disconnect
        [[maybe_unused]] auto findIter = m_sequenceEntityIds.find(sequenceEntityId);
        AZ_Assert(findIter != m_sequenceEntityIds.end(), "A sequence not connected to SequenceAgentComponent on %s is requesting a disconnection", GetEntity()->GetName().c_str());

        m_sequenceEntityIds.erase(sequenceEntityId);

        // Disconnect from the bus between the SequenceComponent and me
        // Make a copy because calling BusDisconnect destroy the current bus id
        const SequenceAgentEventBusId busIdToDisconnectCopy = *busIdToDisconnect;
        EditorSequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(busIdToDisconnectCopy);
        SequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(busIdToDisconnectCopy);

        if (m_sequenceEntityIds.size())
        {
            return;
        }

        AZ::EntityId curEntityId = GetEntityId();

        AZ_Trace("EditorSequenceAgentComponent","DisconnectSequence(): removing self from entity %s, %s.", curEntityId.ToString().c_str(), entity->GetName().c_str());
        // This component was created indirectly via user actions in EditorSequenceComponent,
        // so temporary disable undo / redo for its destruction adding EntityId to the ignored list, to bypass possible undo / redo errors.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddIgnoredEntity, curEntityId);

        // remove this SequenceAgent from this entity if no sequenceComponents are connected to it
        AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::RemoveComponents, AZ::Entity::ComponentArrayType{this});        

        // Remove EntityId from the ignored list to return to standard undo / redo pipeline.
        // This call is mandatory after the above AddIgnoredEntity() call which was intended to disable undo / redo only temporary.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::RemoveIgnoredEntity, curEntityId);

        // Let any currently-active undo operations know that this entity has changed state.
        auto undoCacheInterface = AZ::Interface<AzToolsFramework::UndoSystem::UndoCacheInterface>::Get();
        if (undoCacheInterface)
        {
            undoCacheInterface->UpdateCache(curEntityId);
        }

        // CAUTION!
        // THIS CLASS INSTANCE IS NOW DEAD DUE TO DELETION BY THE ENTITY DURING RemoveComponents!
    }

    void EditorSequenceAgentComponent::ConnectAllSequences()
    {
        // Connect all buses
        for (auto iter = m_sequenceEntityIds.begin(); iter != m_sequenceEntityIds.end(); iter++)
        {
            SequenceAgentEventBusId busIdToConnect(*iter, GetEntityId());
            EditorSequenceAgentComponentRequestBus::MultiHandler::BusConnect(busIdToConnect);
            SequenceAgentComponentRequestBus::MultiHandler::BusConnect(busIdToConnect);
        }
    }

    void EditorSequenceAgentComponent::DisconnectAllSequences()
    {
        // disconnect all buses
        for (auto iter = m_sequenceEntityIds.begin(); iter != m_sequenceEntityIds.end(); iter++)
        {
            SequenceAgentEventBusId busIdToDisconnect(*iter, GetEntityId());
            EditorSequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(busIdToDisconnect);
            SequenceAgentComponentRequestBus::MultiHandler::BusDisconnect(busIdToDisconnect);
        }
    }

    void EditorSequenceAgentComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        SequenceAgentComponent *sequenceAgentComponent = gameEntity->CreateComponent<SequenceAgentComponent>();
        if (sequenceAgentComponent)
        {
            // TODO: when we have code which only allows animation of properties that are common between the SequenceAgent and EditorSequenceAgent,
            // transfer the mappings to the game behaviors here
            sequenceAgentComponent->m_sequenceEntityIds = m_sequenceEntityIds;
        }
    }

    void EditorSequenceAgentComponent::GetAllAnimatableProperties(IAnimNode::AnimParamInfos& properties, AZ::ComponentId componentId)
    {       
        // add all properties found during Activate() that match the given componentId
        for (auto propertyIter = m_addressToBehaviorVirtualPropertiesMap.begin(); propertyIter != m_addressToBehaviorVirtualPropertiesMap.end(); propertyIter++)
        {
            if (propertyIter->first.GetComponentId() == componentId)
            {
                AZ::BehaviorEBus::VirtualProperty* virtualProperty = propertyIter->second;
                // all behavior properties as string params with the virtual property name as the string
                IAnimNode::SParamInfo paramInfo;

                // by default set up paramType as an AnimParamType::ByString with the name as the Virtual Property name
                paramInfo.paramType = propertyIter->first.GetVirtualPropertyName();

                // check for paramType specialization attributes on the getter method of the virtual property. if found, reset
                // to the eAnimParamType enum - this leaves the paramType name unchanged but changes the type.
                for (int i = static_cast<int>(virtualProperty->m_getter->m_attributes.size()); --i >= 0;)
                {
                    if (virtualProperty->m_getter->m_attributes[i].first == AZ::Edit::Attributes::PropertyPosition)
                    {
                        paramInfo.paramType = AnimParamType::Position;
                        break;
                    }
                    else if (virtualProperty->m_getter->m_attributes[i].first == AZ::Edit::Attributes::PropertyRotation)
                    {
                        paramInfo.paramType = AnimParamType::Rotation;
                        break;
                    }
                    else if (virtualProperty->m_getter->m_attributes[i].first == AZ::Edit::Attributes::PropertyScale)
                    {
                        paramInfo.paramType = AnimParamType::Scale;
                        break;
                    }
                    else if (virtualProperty->m_getter->m_attributes[i].first == AZ::Edit::Attributes::PropertyHidden)
                    {
                        paramInfo.flags = static_cast<IAnimNode::ESupportedParamFlags>(paramInfo.flags | IAnimNode::eSupportedParamFlags_Hidden);
                        break;
                    }
                }

                properties.push_back(paramInfo);
            }
        }
    }

    void EditorSequenceAgentComponent::GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& animatableComponentIds)
    {
        AZStd::set<AZ::ComponentId> appendedComponentIds;

        // go through all known properties found during Activate() - insert all unique components found
        for (auto address = m_addressToBehaviorVirtualPropertiesMap.begin(); address != m_addressToBehaviorVirtualPropertiesMap.end(); address++)
        {
            // only append component if it's not already been appended
            auto findIter = appendedComponentIds.find(address->first.GetComponentId());
            if (findIter == appendedComponentIds.end())
            {
                animatableComponentIds.push_back(address->first.GetComponentId());   
                appendedComponentIds.insert(address->first.GetComponentId());
            }
        }
    }

    AZ::Uuid EditorSequenceAgentComponent::GetAnimatedAddressTypeId(const AnimatablePropertyAddress& animatableAddress)
    {
        return GetVirtualPropertyTypeId(animatableAddress);
    }

    void EditorSequenceAgentComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const AnimatablePropertyAddress& animatableAddress)
    {
        SequenceAgent::GetAnimatedPropertyValue(returnValue, GetEntityId(), animatableAddress);
    }

    bool EditorSequenceAgentComponent::SetAnimatedPropertyValue(const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        return SequenceAgent::SetAnimatedPropertyValue(GetEntityId(), animatableAddress, value);
    }

    void EditorSequenceAgentComponent::GetAssetDuration(AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)
    {
        SequenceAgent::GetAssetDuration(returnValue, componentId, assetId);
    }

} // namespace Maestro
